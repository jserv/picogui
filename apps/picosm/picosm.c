//PicoGUI session manager

#include <stdio.h>
#include <pwd.h>
#include <shadow.h>
#include <unistd.h>
#include <picogui.h>

#include "picosm.h"

//----Event Handlers--------------------------------------------------------

int evtLogon(struct pgEvent *evt){
  picosmUI *interface = (picosmUI *)evt->extra;
  char *login, *loginT, *passwd, *passwdT, *passwdSys, *salt = NULL, *rcPath;
  char *args[3];
  struct passwd *userData; 
  struct spwd *sUserData;

  loginT = pgGetString(pgGetWidget(interface->wLogin,PG_WP_TEXT));
  login = (char *)malloc(strlen(loginT));
  strcpy(login, loginT);
  passwdT = pgGetString(pgGetWidget(interface->wPasswd,PG_WP_TEXT));
  passwd = (char *)malloc(strlen(passwdT));
  strcpy(passwd, passwdT);

  if((userData = getpwnam(login)) != NULL){
    if((sUserData = getspnam(login)) != NULL){
      passwdSys = sUserData->sp_pwdp;
    }else{
      passwdSys = userData->pw_passwd;
    }
    if(passwdSys[0] == '$'){
      salt = (char *)malloc(13);
      strncpy(salt, passwdSys, 12);
    }else{
      salt = (char *)malloc(3);
      strncpy(salt, passwdSys, 2);
    }
    if(!strcmp(passwdSys, (char *)crypt(passwd, salt))){
      if(!fork()){
	setuid(userData->pw_uid);
	rcPath = (char *)malloc(strlen(userData->pw_dir)+
				strlen("/.picoguirc"));
	sprintf(rcPath, "%s/.picoguirc", userData->pw_dir); 
	printf("%s\n", rcPath);
	args[0] = "/bin/sh";
	args[1] = rcPath;
	args[2] = NULL;
	execve("/bin/sh", args, NULL); 
      }else{
	exit(0);
      }
    } 
  }

  pgMessageDialog("Error", "Login failed!", 0);
  evtClear(evt);

  if(salt)
    free(salt);
  free(login);
  free(passwd);

  return 1;
}

int evtClear(struct pgEvent *evt){
  picosmUI *interface = (picosmUI *)evt->extra;

  pgEnterContext();
  pgSetWidget(interface->wPasswd,
	      PG_WP_TEXT, pgNewString(""),
	      0);
  pgSetWidget(interface->wLogin,
	      PG_WP_TEXT, pgNewString(""),
	      0);
  pgFocus(interface->wLogin);
  pgLeaveContext();

  return 1;
}

int evtLogin(struct pgEvent *evt){
  picosmUI *interface = (picosmUI *)evt->extra;

  /* Advance focus to the password field if the username field is nonempty */
  if (pgGetString(pgGetWidget(interface->wLogin,PG_WP_TEXT))[0])
    pgFocus(interface->wPasswd);

  return 1;
}

int evtReboot(struct pgEvent *evt){
  printf("Bob says: ReBoot!\n");
  return 1;
}

int evtShutdown(struct pgEvent *evt){
  printf("Call init 6\n");
  return 1;
}

//----UI Functions----------------------------------------------------------

picosmUI *buildUI(void){
  picosmUI *newUI = (picosmUI *)malloc(sizeof(picosmUI));

  if(!(newUI->wApp = pgFindWidget("PSMApp"))){
    newUI->wApp = pgCreateWidget(PG_WIDGET_DIALOGBOX);
    pgSetWidget(newUI->wApp,
		PG_WP_NAME,pgNewString("PSMApp"),
		PG_WP_TEXT,pgNewString("PicoGUI Login"),
		0);
  }

  if(!(newUI->wButtonBox = pgFindWidget("PSMButtonBox"))){
    newUI->wButtonBox = pgNewWidget(PG_WIDGET_TOOLBAR, PG_DERIVE_INSIDE, 
				    newUI->wApp);
    pgSetWidget(newUI->wButtonBox,
		PG_WP_NAME,pgNewString("PSMButtonBox"),
		PG_WP_SIDE, PG_S_BOTTOM,
		0);
  }

  if(!(newUI->wPasswdBox = pgFindWidget("PSMPasswdBox"))){
    newUI->wPasswdBox = pgNewWidget(PG_WIDGET_BOX, PG_DERIVE_INSIDE, 
				    newUI->wApp);
    pgSetWidget(newUI->wPasswdBox,
		PG_WP_NAME,pgNewString("PSMPasswdBox"),
		PG_WP_TRANSPARENT, 1,
		0);
  }

  if(!(newUI->wLoginBox = pgFindWidget("PSMLoginBox"))){
    newUI->wLoginBox = pgNewWidget(PG_WIDGET_BOX, PG_DERIVE_INSIDE, 
				   newUI->wApp);
    pgSetWidget(newUI->wLoginBox,
		PG_WP_NAME,pgNewString("PSMLoginBox"),
		PG_WP_TRANSPARENT, 1,
		0);
  }

  if(!(newUI->wLoginLabel = pgFindWidget("PSMLoginLabel"))){
    newUI->wLoginLabel = pgNewWidget(PG_WIDGET_LABEL, PG_DERIVE_INSIDE, 
				     newUI->wLoginBox);
    pgSetWidget(newUI->wLoginLabel, 
		PG_WP_NAME,pgNewString("PSMLoginLabel"),
		PG_WP_TEXT, pgNewString("Login: "),
		PG_WP_ALIGN, PG_A_RIGHT,
		PG_WP_SIDE, PG_S_ALL,
		0);
  }

  if(!(newUI->wLogin = pgFindWidget("PSMLogin"))){
    newUI->wLogin = pgNewWidget(PG_WIDGET_FIELD, PG_DERIVE_INSIDE, 
				newUI->wLoginBox);
    pgSetWidget(newUI->wLogin, 
		PG_WP_NAME,pgNewString("PSMLogin"),
		PG_WP_SIDE, PG_S_RIGHT,
		PG_WP_SIZEMODE, PG_SZMODE_PERCENT,
		PG_WP_SIZE, 70,
		0);
    pgFocus(newUI->wLogin);
  }
    
  if(!(newUI->wPasswdLabel = pgFindWidget("PSMPasswdLabel"))){
    newUI->wPasswdLabel = pgNewWidget(PG_WIDGET_LABEL, PG_DERIVE_INSIDE, 
				      newUI->wPasswdBox);
    pgSetWidget(newUI->wPasswdLabel, 
		PG_WP_NAME,pgNewString("PSMPasswdLabel"),
		PG_WP_TEXT, pgNewString("Password: "),
		PG_WP_ALIGN, PG_A_RIGHT,
		PG_WP_SIDE, PG_S_ALL,
		0);
  }

  if(!(newUI->wPasswd = pgFindWidget("PSMPasswd"))){
    newUI->wPasswd = pgNewWidget(PG_WIDGET_FIELD, PG_DERIVE_INSIDE, 
				 newUI->wPasswdBox);
  
    pgSetWidget(newUI->wPasswd, 
		PG_WP_NAME,pgNewString("PSMPasswd"),
		PG_WP_SIDE, PG_S_RIGHT,
		PG_WP_SIZEMODE, PG_SZMODE_PERCENT,
		PG_WP_PASSWORD, '*',
		PG_WP_SIZE, 70,
		0);
  }

  if(!(newUI->wClear = pgFindWidget("PSMClear"))){
    newUI->wClear = pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, 
				newUI->wButtonBox);
    pgSetWidget(newUI->wClear,
		PG_WP_NAME,pgNewString("PSMClear"),
		PG_WP_ALIGN, PG_A_CENTER,
		PG_WP_SIDE, PG_S_RIGHT,
		PG_WP_TEXT, pgNewString("Clear"),
		0);
  }

  if(!(newUI->wLogon = pgFindWidget("PSMLogon"))){
    newUI->wLogon = pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_INSIDE,
				newUI->wButtonBox);
    pgSetWidget(newUI->wLogon,
		PG_WP_NAME,pgNewString("PSMLogon"),
		PG_WP_ALIGN, PG_A_CENTER,
		PG_WP_SIDE, PG_S_RIGHT,
		PG_WP_TEXT, pgNewString("Log On"),
		0);
  }

  if(!(newUI->wReboot = pgFindWidget("PSMReboot"))){
    newUI->wReboot = pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, 
				 newUI->wButtonBox);
    pgSetWidget(newUI->wReboot,
		PG_WP_NAME,pgNewString("PSMReboot"),
		PG_WP_ALIGN, PG_A_CENTER,
		PG_WP_SIDE, PG_S_LEFT,
		PG_WP_TEXT, pgNewString("Reboot"),
		0);
  }

  if(!(newUI->wPowerOff = pgFindWidget("PSMPowerOff"))){
    newUI->wPowerOff = pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, 
				   newUI->wButtonBox);
    pgSetWidget(newUI->wPowerOff,
		PG_WP_NAME,pgNewString("PSMPowerOff"),
		PG_WP_ALIGN, PG_A_CENTER,
		PG_WP_SIDE, PG_S_LEFT,
		PG_WP_TEXT, pgNewString("Power Off"),
		0);
  }

  return newUI;
}

void bindUI(picosmUI *interface){
  pgBind(interface->wLogon, PG_WE_ACTIVATE, &evtLogon, interface);
  pgBind(interface->wClear, PG_WE_ACTIVATE, &evtClear, interface);
  pgBind(interface->wReboot, PG_WE_ACTIVATE, &evtReboot, NULL);
  pgBind(interface->wPowerOff, PG_WE_ACTIVATE, &evtShutdown, NULL);
  pgBind(interface->wLogin, PG_WE_ACTIVATE, &evtLogin, interface);
  pgBind(interface->wPasswd, PG_WE_ACTIVATE, &evtLogon, interface);
}

int main(int argc, char **argv){
  picosmUI *interface;
  FILE *existCheck;

  pgInit(argc, argv);

  if(existCheck = fopen("picosm.wt", "r")){
    fclose(existCheck);
    pgDup(pgLoadWidgetTemplate(pgFromFile("picosm.wt")));
  }
  interface = buildUI();
  bindUI(interface);

  pgEventLoop();
  
  free(interface);
  return 0;
}
