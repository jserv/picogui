#include <picogui.h>
#include <pim.h>

int setDate(date *ptr) {

  /*   question - how do I get date displayed in title bar of 
                  calendar dialog to change as I change dates?
  */

  int retcode=0;

  //pgEnterContext(); // needed?

  retcode=pgDatePicker(&(ptr->year),&(ptr->month),&(ptr->day),"PicoPIM");  

  //pgLeaveContext(); //needed?

  return 0;
}



