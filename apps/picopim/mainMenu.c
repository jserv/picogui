//#include <picogui.h>
#include <pim.h>

/*
   This sets up the PIM main screen.  First, it loads the pim data linked list from the pimData.txt file.
   Then it passes control back and forth betweeen itself and PIM modules (like calendar and todo).   Finally, 
   it writes data back to pimData.txt when a PIM module (capable of changing data) is exited.

*/

struct _entry *entryListHead; /* ptr to beginning of pim data linked list */


pghandle box;

//entry dummyHead;

int loadData(){

  /*
     Read pimData.txt.  Malloc the pim data list entries.  Load them up and link them together.  If 
     pimData.txt is not found, the list will remain empty.
  */

  FILE *pimFile;
  entry pimData;
  struct _entry *tmp,*new;
  int i;
  int todoEnt=0;

  entryListHead=(entry*)malloc(sizeof(entry));
  pimFile=fopen("pimData.txt","r");
  if (pimFile != NULL){
    fread(&pimData,sizeof(pimData),1,pimFile);


    tmp=(entry*)malloc(sizeof(entry));
    memset(tmp,'\0',sizeof(entry));
    memcpy(&(tmp->handle),&(pimData.handle),
           sizeof(pimData.handle));
    memcpy(tmp->data,&pimData.data[0],strlen(pimData.data));
    tmp->delete=0;
    tmp->next=NULL;
    tmp->prev=NULL;
    entryListHead=tmp;
      if (tmp->handle.entryType == TODO){
           printf("data = %s\n",tmp->data);
           todoEnt++;
      }

    i=0;
    fread(&pimData,sizeof(pimData),1,pimFile);
    while (!feof(pimFile)){

      new=(entry*)malloc(sizeof(entry));
      memcpy(&(new->handle),&(pimData.handle),
             sizeof(pimData.handle));
      memcpy(new->data,pimData.data,strlen(pimData.data));
      new->delete=0;
      new->next=NULL;

      printf("data = %s\n",new->data);
      tmp->next=new;
      new->prev=tmp;
      tmp=tmp->next;

      i++;
      fread(&pimData,sizeof(pimData),1,pimFile);
    }

    printf("number entries in list = %d\n",i);
    printf("number todo entries in list = %d\n",todoEnt);

    printf("entryListHead = %d\n",entryListHead); 
    fclose(pimFile);
  }
}

int saveData(){
  /*
      Write the pim data list back to pimData.txt, skipping any entries marked for deletion.
  */

  FILE *pimFile;
  entry pimData;
  struct _entry *tmp,*new;
  int i;
  int todoEnt=0;

  printf("in saveData()\n");
  pimFile=fopen("pimData.txt","w");
  tmp=entryListHead;
  while (tmp != NULL){
    if (tmp->delete != 1){
       memset(&pimData,'\0',sizeof(pimData));
       memcpy(&(pimData.handle),&(tmp->handle),sizeof(pimData.handle));
       memcpy(&pimData.data[0],tmp->data,strlen(tmp->data));       
       fwrite(&pimData,sizeof(pimData),1,pimFile);
    }
    tmp=tmp->next;
  }
  fclose(pimFile);
}

int calendarModule(struct pgEvent *evt){
  /* 
     pops up the calendar.  right now the returned date is saved, but not used anywhere.
  */
  
  setDate(&pimDate);
  printf("new date year = %d month = %d day = %d\n",
         pimDate.year, pimDate.month, pimDate.day);    
  return 0;
}

int todoModule(struct pgEvent *evt){
  /*
     starts the to do list module
   */
  pgEnterContext();
  todoMain(evt,box);
  return 0;
}

int main(int argc, char **argv) {



  /*
        load the data.  slap up some buttons and a toolbar.  kick off the event loop.
  */

  pghandle toolbar;
  pghandle button1, button2, button3, button4;

  FILE *pimFile;
  entry pimData;
  struct _entry *tmp,*new;
  int i;
  // DEBUG VARS
  int todoEnt=0;

  entryListHead=NULL;
  loadData();
  /*  entryListHead=(entry*)malloc(sizeof(entry));
  pimFile=fopen("pimData.txt","r");
  fread(&pimData,sizeof(pimData),1,pimFile);


  tmp=(entry*)malloc(sizeof(entry));
  memcpy(&(tmp->handle),&(pimData.handle),
         sizeof(pimData.handle));
  memcpy(tmp->data,&pimData.data[0],strlen(pimData.data));
  tmp->delete=0;
  tmp->next=NULL;
  tmp->prev=NULL;
  entryListHead=tmp;
    if (tmp->handle.entryType == TODO){
           printf("data = %s\n",tmp->data);
      todoEnt++;
    }

  i=0;
  fread(&pimData,sizeof(pimData),1,pimFile);
  while (!feof(pimFile)){

    new=(entry*)malloc(sizeof(entry));
    memcpy(&(new->handle),&(pimData.handle),
           sizeof(pimData.handle));
    memcpy(new->data,pimData.data,strlen(pimData.data));
    new->delete=0;
    new->next=NULL;

    printf("data = %s\n",new->data);

    tmp->next=new;
    new->prev=tmp;
    tmp=tmp->next;

    i++;
    fread(&pimData,sizeof(pimData),1,pimFile);
  }

  printf("number entries in list = %d\n",i);
  printf("number todo entries in list = %d\n",todoEnt);

   printf("entryListHead = %d\n",entryListHead); */
   
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"PIM MAIN",0);


   //pgEnterContext();





    
   toolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
   pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_TOP,0);

   button3 = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,toolbar);
   pgSetWidget(button3,
               PG_WP_TEXT, pgNewString("Memo"),
               PG_WP_SIDE, PG_S_LEFT,0);
   //   pgBind(PGDEFAULT,PG_WE_ACTIVATE,&memoModule,NULL);

   button2 = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,toolbar);
   pgSetWidget(button2,
               PG_WP_TEXT, pgNewString("To Do"),
               PG_WP_SIDE, PG_S_LEFT,0);
   pgBind(PGDEFAULT,PG_WE_ACTIVATE,&todoModule,NULL);

   button1 = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,toolbar);
   pgSetWidget(button1,
               PG_WP_TEXT, pgNewString("Calendar"),
               PG_WP_SIDE, PG_S_LEFT,0);
   pgBind(PGDEFAULT,PG_WE_ACTIVATE,&calendarModule,NULL);

   box=pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_AFTER,toolbar);
  pgSetWidget(box,
	      PG_WP_SIDE,PG_S_ALL,
	      0);      
   printf("right before pgEventLoop\n");
   pgEventLoop();
   printf("right after pgEventLoop\n");

   //pgLeaveContext();


   return 0;
}

