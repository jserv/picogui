
#include <picogui.h>

#define  TODO 1
#define  MEMO 2
#define SCHED 3

typedef struct{
  int year,month,day;
} date;

date pimDate;
date systemDate;

typedef struct{
  date entryDate;
  int  entryType;
} key;

typedef struct _entry{
  key handle;
  pghandle widgetHandle;
  char data[160];
  int delete;
  struct _entry *next;
  struct _entry *prev;
} entry; /*  an entry in the pim data linked list */


