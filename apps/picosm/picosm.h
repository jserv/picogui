typedef struct {

  pghandle wApp;

  //Boxes
  pghandle wLoginBox;
  pghandle wPasswdBox;
  pghandle wButtonBox;
  
  //Login
  pghandle wLogin;
  pghandle wLoginLabel;
  
  //Passwd
  pghandle wPasswd;
  pghandle wPasswdLabel;

  //Buttons
  pghandle wLogon;
  pghandle wClear;
  pghandle wReboot;
  pghandle wPowerOff;
} picosmUI;
