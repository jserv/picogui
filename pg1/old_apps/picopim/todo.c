
/* 

   FIXME - adding entries does not resize listPopup and previous items are
           not seen.  must exit todo and re-enter. 
   FIXME - If a new entry is longer than the others, its displayed with the 
           wrong width.  You must restart todo to get the corrected width. 
   FIXME - Disable buttons in background.

*/

#include <pim.h>

extern int saveData();

extern struct _entry *entryListHead; /* pointer to main data list */
extern pghandle box;

typedef struct _dispEntry{
  pghandle handle;
  char *text;
  struct _entry *entryPtr;
  struct _dispEntry *next;
  struct _dispEntry *prev;
  int changed;
} dispEntry;        /* Represents a PIM entry and the pghandle 
                      of its screen widget.  Gotta add the key to this. 
                      Or is a key necessary? 

                      The display dataset is a linked list of these structures.  The dataset is built from 
                      TODO entries found in the main data list.  */

struct _dispEntry *dispListHead; /* ptr to start of display dataset. */
struct _dispEntry *dispListEnd;  /* ptr to end of display dataset    */
struct _dispEntry *ptrToEntryForEdit; /* this points to the display dataset entry we are working with.
                                         this isn't confined to editing anymore. */


pghandle widgetHandle; /* pghandle of display list entry being edited or
                          viewed.  */


char *originalString,     // original entry being displayed
     *replacementString;  // entry after view/edit.  these can probably
                          // be made local somewhere. 

pghandle listPopup,       // popup for list of pim entries
            wBox,         // container for pim entries
         listScroll,      // make it scrollable 
         listLabel,       // give it a title
         toolbar,         // listPopup toolbar for exit button et al
         button1,         // listPopup's exit button
            wItem;        /* temp var to hold handle prior to recording it 
			     int display list*/ 


pghandle editWidgetPopup,   // popup to hold edit/view widgets
         editWidget,        // widget to hold text for editing/viewing
         toolbar2,          // toolbar for edit screen
         editWidgetExitBtn, // indicates you are done 
         addButton,
         deleteButton,
         cancelButton;

pghandle entryDetailExitButton;

int dataEntry(struct pgEvent *evt);

#ifdef DEBUG
void dumpDispList(){


   dispEntry *ptr;

   ptr=dispListHead;
   while(ptr != NULL){
     printf("\n\n\n");
     printf("ptr = %d\n",ptr);
     printf("display entry handle = %d\n",ptr->handle);
     printf("                text = %s\n",ptr->text);
     printf("                next = %d\n",ptr->next);
     printf("                prev = %d\n",ptr->prev);
     printf(" data list entry ptr = %d\n",ptr->entryPtr);
     ptr=ptr->next;
   }

}

void dumpEntry(dispEntry *ptr){
  printf("in dumEntry\n");
     printf("\n\n\n");
     printf("ENTRY DATA FOR ptr = %d\n",ptr);
     printf("display entry handle = %d\n",ptr->handle);
     printf("                text = %s\n",ptr->text);
     printf("                next = %d\n",ptr->next);
     printf("                prev = %d\n",ptr->prev);
     printf(" data list entry ptr = %d\n",ptr->entryPtr);
}

#endif

int swap(struct pgEvent *evt){

  /* get the string from the editing widget.  place in display list and
     data list. */

  char *tmp,*replacementString;
  pghandle text;

  // get the text from the edit widget and place in replacementString 
  text=evt->from;
  tmp=pgGetString(pgGetWidget(text,PG_WP_TEXT));
  replacementString=(char *)malloc(strlen(tmp)+1);
  memset(replacementString,'\0',strlen(tmp)+1);
  memcpy(replacementString,tmp,strlen(tmp));

  // put replacementString data into display list AND data list 
  free(ptrToEntryForEdit->text);
  ptrToEntryForEdit->text=(char *)malloc(strlen(tmp)+1);
  memset(ptrToEntryForEdit->text,'\0',strlen(tmp)+1);
  memcpy(ptrToEntryForEdit->text,replacementString,strlen(tmp));
  memcpy(ptrToEntryForEdit->entryPtr->data,replacementString,strlen(tmp));
  free(replacementString);


}

int insert(struct pgEvent *evt){

  /* insert a new entry in the display list AND data list */

  char *tmp,*replacementString;
  pghandle text;
  dispEntry *ptr,*ptr2;

  // get the new entry text. 
  text=evt->from;
  tmp=pgGetString(pgGetWidget(text,PG_WP_TEXT));

  // create new display list and data list entries
  ptr=(dispEntry*)malloc(sizeof(dispEntry));
  ptr->entryPtr=(struct _entry *)malloc(sizeof(entry));
  memset(ptr->entryPtr,'\0',sizeof(entry));  //why am i only initializing this?
  //ptr->entryPtr->handle = ?;
  //ptr->entryPtr->widgetHandle = ?;

  //load up the data list entry and insert at beginning of data list
  memcpy(ptr->entryPtr->data,tmp,strlen(tmp));
  ptr->entryPtr->delete=0;
  ptr->entryPtr->handle.entryType=TODO;
  ptr->entryPtr->next=entryListHead;
  entryListHead=ptr->entryPtr;

  /*load up display list entry and partially insert at beginning of display 
    list */
  ptr->text=(char *)malloc(strlen(tmp)+1);
  ptr->changed=0;
  ptr->next=NULL;
  ptr->prev=NULL;
  memset(ptr->text,'\0',strlen(tmp)+1);
  memcpy(ptr->text,tmp,strlen(tmp));
  ptr->next=dispListHead;

#ifdef DEBUG
  printf("entry just added\n");
  dumpEntry(ptr);
#endif

  // handles adding new entry to display list if it is not the only item
  if (dispListHead != NULL){
     dispListHead->prev=ptr;
  }

  // finish inserting at beginning of display list
  dispListHead=ptr;

  // handles adding new entry to display list if it IS the only item
  if (dispListEnd == NULL){
    dispListEnd=ptr;
  }



}

int deleteRtne(struct pgEvent *evt){ 

  /* remove entry from display list and data list.  update list start and 
     end pointers */

struct _dispEntry *entryBefore, *entryAfter; 

if (ptrToEntryForEdit != NULL){
  printf("deleteRtne deleting %s\n",ptrToEntryForEdit->text);

  entryBefore=ptrToEntryForEdit->prev;
  entryAfter=ptrToEntryForEdit->next;

  if ((entryBefore == NULL) && (entryAfter == NULL)){
    // deleting only entry in list
    dispListHead=NULL;
    dispListEnd=NULL;
  }
  else{
  if (entryBefore == NULL){
    // entry to delete is 1st in list
    entryAfter->prev=NULL;
    dispListHead=entryAfter;
  }
  else{
    if (entryAfter == NULL){
      // entry to delete is last in list
      entryBefore->next=NULL;
      dispListEnd=entryBefore;
    }
    else{
       // entry to delete is in middle of list
       entryBefore->next=ptrToEntryForEdit->next;
       entryAfter->prev=ptrToEntryForEdit->prev;
    }
  }
  }

  pgDelete(ptrToEntryForEdit->handle);
  ptrToEntryForEdit->entryPtr->delete=1;

  if (ptrToEntryForEdit->entryPtr->next != NULL){
   ptrToEntryForEdit->entryPtr->next->prev=ptrToEntryForEdit->entryPtr->prev;
  }
  else{
    ptrToEntryForEdit->entryPtr->prev->next=NULL;
  }

  if (ptrToEntryForEdit->entryPtr->prev != NULL){
   ptrToEntryForEdit->entryPtr->prev->next=ptrToEntryForEdit->entryPtr->next;
  }
  else{
    ptrToEntryForEdit->entryPtr->next->prev=NULL;
  }

  if (ptrToEntryForEdit->entryPtr == entryListHead){
    entryListHead=ptrToEntryForEdit->entryPtr->next;
  }

  free(ptrToEntryForEdit->entryPtr);    
  free(ptrToEntryForEdit); //will this free everything in the struct?
}

}



int exitRtne(struct pgEvent *evt) {

  /* get rid of all entries on screen and everything in listPopup

   */

  struct _dispEntry *ptr;

  printf("in exitRtne, about to leave context\n");
  ptr=dispListHead;
  while (ptr!=NULL){
    if (ptr->handle != 0){
      pgDelete(ptr->handle);  //deleting widget handle.  is new entry in list
                              //yet?  this could be bad.
    }
    ptr=ptr->next;
  }

  //pgDelete(listLabel);    now why did I get an error when I tried this?
  // pgDelete(listPopup); crashed when deleting entries

  /*pgDelete(listScroll);
  pgDelete(wBox);
  pgDelete(toolbar); */

  pgLeaveContext();
  saveData();
  return 0;
}


int editCancel(struct pgEvent *evt) {
  /* get rid of edit widgets */
  pgDelete(editWidget);
  pgDelete(toolbar2);
  pgDelete(editWidgetPopup);
}

int exitRtne2(struct pgEvent *evt) {

  // forces redisplay of display list.  deletes the editing and list display
  //   widgets.  recreates the list display widgets containing the updated
  //   contents of dispArr.  I couldn't figure out how to update just one
  //   entry.  so I used brute force.
  //
  //   there really aren't enough good things said about brute force ;-)
  //

  int i=0;
  struct _dispEntry *ptr;

  pgDelete(editWidget);
  pgDelete(toolbar2);
  pgDelete(editWidgetPopup);
  //pgDelete(editWidgetExitBtn); can't delete this, its is responsible for
  //                               getting us here.  why you can delete the 
  //                               stuff that contains it i don't know.

  ptr=dispListHead;
  while (ptr!=NULL){
    if (ptr->handle != 0){
#ifdef DEBUG
      printf("exitRtne2 deleting text=%s handle=%d\n",ptr->text,ptr->handle); 
#endif
      pgDelete(ptr->handle);  //deleting widget handle.  is new entry in list
                              //yet?  this could be bad.
    }
    ptr=ptr->next;
  }

  ptr=dispListEnd;
  while (ptr!=NULL) {
    printf("in exitRtne2 text = %s\n",ptr->text);
    ptr->handle = pgNewWidget(PG_WIDGET_LISTITEM,
			      i ? PGDEFAULT : PG_DERIVE_INSIDE,
			      i ? PGDEFAULT : wBox);  
    pgSetWidget(ptr->handle,
                PG_WP_TEXT,pgNewString(ptr->text),
                0);
    pgBind(ptr->handle,PG_WE_ACTIVATE,&dataEntry,NULL);    
    ptr=ptr->prev;
    printf("made it past ptr=ptr->prev\n");
  }
  printf("made it out of loop\n");
#ifdef DEBUG
  printf("list after redisplay loop\n");
  dumpDispList();
#endif
      
  return 0;
}


int dataEntry(struct pgEvent *evt) {

  /*
        the user selects an entry from the
        list entry screen and this routine it invoked.  it grabs
        the text from the selected entry, creates
        and manages widgets to allow editing of
        the entry.
  */
 
  pghandle wText,editBox;

  char *tmp;
  int found=0;
  int i=0;

  /* so why not have exitRtne2 goof with widgetHandle
     instead of deleting everything */  

  printf("in dataEntry\n");
  widgetHandle=evt->from;

  ptrToEntryForEdit=dispListHead;
  
  while(!found){
    if (ptrToEntryForEdit->handle == widgetHandle){
      found=1;
    }
    else{
      ptrToEntryForEdit=ptrToEntryForEdit->next;
    }
  }

  tmp=pgGetString(pgGetWidget(widgetHandle,PG_WP_TEXT));
  originalString=(char *)malloc(strlen(tmp)+1);
  memset(originalString,'\0',strlen(tmp)+1);
  memcpy(originalString,tmp,strlen(tmp));

   
  editWidgetPopup=pgNewPopup(PGDEFAULT,PGDEFAULT);

  toolbar2 = pgNewWidget(PG_WIDGET_TOOLBAR,PG_DERIVE_INSIDE,editWidgetPopup);
  pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_BOTTOM,0);

  editBox=pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,editWidgetPopup);

  editWidget=pgNewWidget(PG_WIDGET_FIELD,PG_DERIVE_INSIDE,editBox);
  pgSetWidget(editWidget,
              PG_WP_SIDE,PG_S_BOTTOM,
	      PG_WP_TEXT,pgGetWidget(widgetHandle,PG_WP_TEXT),
	      0);
  pgBind(editWidget,PG_WE_ACTIVATE,&swap,NULL);

  editWidgetExitBtn=pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,
                                toolbar2);
  pgSetWidget(editWidgetExitBtn,
	      PG_WP_TEXT,pgNewString("Exit"),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);
  pgBind(editWidgetExitBtn,PG_WE_ACTIVATE,&exitRtne2,NULL);

  cancelButton=pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,toolbar2);
  pgSetWidget(cancelButton,
	      PG_WP_TEXT,pgNewString("Cancel"),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);
  pgBind(cancelButton,PG_WE_ACTIVATE,&editCancel,NULL);

  deleteButton=pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,toolbar2);
  pgSetWidget(deleteButton,
	      PG_WP_TEXT,pgNewString("Delete"),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);
  pgBind(deleteButton,PG_WE_ACTIVATE,&deleteRtne,NULL);

  return 0;
}

int dummyRtne(struct pgEvent *evt) {
  printf("this routine does nothing\n");
  return 0;
}

int addRtne(struct pgEvent *evt) {

  /* display edit widgets containing selected data.  get changes and
     insert in lists */

  editWidgetPopup=pgNewPopup(PGDEFAULT,PGDEFAULT);

  toolbar2 = pgNewWidget(PG_WIDGET_TOOLBAR,PG_DERIVE_INSIDE,editWidgetPopup);
  pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_BOTTOM,0);

  editWidget=pgNewWidget(PG_WIDGET_FIELD,PG_DERIVE_INSIDE,editWidgetPopup);
  pgSetWidget(editWidget,
	      //PG_WP_TEXT,pgGetWidget(widgetHandle,PG_WP_TEXT),
	      0);
  pgBind(editWidget,PG_WE_ACTIVATE,&insert,NULL);

  editWidgetExitBtn=pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,
                                toolbar2);
  pgSetWidget(editWidgetExitBtn,
	      PG_WP_TEXT,pgNewString("Exit"),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);
  pgBind(editWidgetExitBtn,PG_WE_ACTIVATE,&exitRtne2,NULL);

  deleteButton=pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,toolbar2);
  pgSetWidget(deleteButton,
	      PG_WP_TEXT,pgNewString("Cancel"),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);
  pgBind(deleteButton,PG_WE_ACTIVATE,&deleteRtne,NULL);

  return 0;
}



int todoMain(struct pgEvent *evt){
   
  int ac;
  char **av;
  struct _entry *tmp;
  extern struct _entry *entryListHead;
  int i;
  struct _dispEntry *ptr, *listPtr;



  listPopup=pgNewPopupAt(PG_POPUP_CENTER,PG_POPUP_CENTER,PGDEFAULT,PGDEFAULT);
  wBox = pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,listPopup);





  listScroll=pgNewWidget(PG_WIDGET_SCROLL, PG_DERIVE_BEFORE, wBox);
  pgSetWidget(listScroll,
	      PG_WP_BIND, wBox,
	      0);


  toolbar = pgNewWidget(PG_WIDGET_TOOLBAR,PG_DERIVE_INSIDE,listPopup);  
   
  pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_BOTTOM,0);

   tmp=entryListHead;
   dispListHead=NULL;
   i=0;
   while (tmp != NULL){

     if (tmp->handle.entryType == TODO){

       wItem = pgNewWidget(PG_WIDGET_LISTITEM,
	  		    i ? PGDEFAULT : PG_DERIVE_INSIDE,
			    i ? PGDEFAULT : wBox);

         ptr=(dispEntry*)malloc(sizeof(dispEntry));
         ptr->text=(char *)malloc(strlen(tmp->data)+1);
         ptr->changed=0;
         memset(ptr->text,'\0',strlen(tmp->data)+1);
         ptr->next=NULL;
         ptr->prev=NULL;
         ptr->entryPtr=tmp;
         memcpy(ptr->text,tmp->data,strlen(tmp->data));


         pgSetWidget(wItem,
                PG_WP_TEXT,pgNewString(ptr->text),
                0);
         tmp->widgetHandle=wItem;
         ptr->handle=wItem;
         pgBind(wItem,PG_WE_ACTIVATE,&dataEntry,NULL);
         //pgBind(wItem,PG_WE_PNTR_DOWN,&dataEntry,NULL);

         if (dispListHead==NULL){
          dispListHead=ptr;
          dispListEnd=ptr;
          listPtr=ptr;
         }
         else{
          ptr->prev=listPtr;
          listPtr->next=ptr;
          listPtr=listPtr->next;
          dispListEnd=ptr;
         }

  
         i++;
     }    
     tmp=tmp->next;
     }

  listLabel=pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,listPopup);
  pgSetWidget(listLabel,
	      PG_WP_TEXT,pgNewString("To Do"),
              PG_WP_SIDE,PG_S_TOP,
      	      0);

#ifdef DEBUG
   printf("initial list\n");
  dumpDispList();
#endif

  entryDetailExitButton=pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,toolbar);
  pgSetWidget(entryDetailExitButton,
	      PG_WP_TEXT,pgNewString("Exit"),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);
  pgBind(entryDetailExitButton,PG_WE_ACTIVATE,&exitRtne,NULL);      

      

  addButton=pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,toolbar);
  pgSetWidget(addButton,
	      PG_WP_TEXT,pgNewString("Add"),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);
  pgBind(addButton,PG_WE_ACTIVATE,&addRtne,NULL);      


   return 0;
}


