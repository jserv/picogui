
#include <stdio.h>
#include <stdlib.h>
#include "pim.h"

/* makes a sample pimData.txt with lots of entries */

main(int argc, char **argv){

  int i;
  entry pimData;
  FILE *dataFile;
  char todoString[81], memoString[81], schedString[81];

  dataFile=fopen("pimData.txt","w");

  pimData.handle.entryDate.year=2002;
  pimData.handle.entryDate.month=5;
  pimData.handle.entryDate.day=0;

  pimData.handle.entryType=TODO;
  for (i=61;i>0;i--){
    pimData.handle.entryDate.day=i;
    sprintf(pimData.data,"to do entry %d1234567890123456\n78901234567890lkdfjgalkjsdf\nlaksdjfoiasdfeoimfnakw",i);
    fwrite(&pimData,sizeof(pimData),1,dataFile);    
  }

  pimData.handle.entryDate.year=2002;
  pimData.handle.entryDate.month=4;
  pimData.handle.entryDate.day=0;     

  pimData.handle.entryType=MEMO;
  for (i=31;i>0;i--){
    pimData.handle.entryDate.day=i;
    sprintf(pimData.data,"memo entry %d",i);
    fwrite(&pimData,sizeof(pimData),1,dataFile);    
  }

  pimData.handle.entryDate.year=2002;
  pimData.handle.entryDate.month=3;
  pimData.handle.entryDate.day=0;

  pimData.handle.entryType=SCHED;

  for (i=31;i>0;i--){
    pimData.handle.entryDate.day=i;
    sprintf(pimData.data,"sched entry %d",i);
    fwrite(&pimData,sizeof(pimData),1,dataFile);    
  }
}
