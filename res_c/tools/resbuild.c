#include <unistd.h>
#include <stdio.h>

#include "res_c.h"

int main(int argc, char **argv){
  int c;
  char *confPath = NULL;
  char *inresPath = NULL;
  char *delresPath = NULL;
  char *setparm = NULL;
  char *unsetparm = NULL;

  while((c = getopt(argc, argv, "f:a:d:s:u:")) != -1){
    switch(c){
    case 'f':
      confPath = optarg;
      break;
    case 'a':
      inresPath = optarg;
      break;
    case 'd':
      delresPath = optarg;
      break;
    case 's':
      setparm = optarg;
      break;
    case 'u':
      unsetparm = optarg;
      break;
    case '?':
      printf("Usage: Later\n");
      return 1;
    }
  }
}
