/* 	$Id: resbuild.c,v 1.2 2002/10/13 12:36:43 carpman Exp $	 */

#ifndef lint
static char vcid[] = "$Id: resbuild.c,v 1.2 2002/10/13 12:36:43 carpman Exp $";
#endif /* lint */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "res_c.h"

int addResource(char *confPath, char *resName, char *resPath){
  resResource *resource;
  
  if(resource = resLoadResource(confPath)){
    resSetProperty(resource, "Resources", resName, resPath);
  }
  resUnloadResource(resource);
}

int setParam(char *confPath, char *paramName, char *paramData){
  resResource *resource;
  
  if(resource = resLoadResource(confPath)){
    resSetProperty(resource, "Parameters", paramName, paramData);
  }
  resUnloadResource(resource);
}

int setBin(char *confPath, char *arch, char *binName){
  resResource *resource;
  
  if(resource = resLoadResource(confPath)){
    resSetProperty(resource, arch, "executable", binName);
  }
  resUnloadResource(resource);
}

int setName(char *confPath, char *appName){
  resResource *resource;
  
  if(resource = resLoadResource(confPath)){
    resSetProperty(resource, "Launcher", "name", appName);
  }
  resUnloadResource(resource);
}

int main(int argc, char **argv){
  int c;
  char *confPath = NULL;
  char *addresPath = NULL;
  char *delresPath = NULL;
  char *setparm = NULL;
  char *unsetparm = NULL;
  char *addBin = NULL;
  char *delBin = NULL;
  char *appName = NULL;
  FILE *confFile;

  while((c = getopt(argc, argv, "f:a:d:s:u:b:e:n:")) != -1){
    switch(c){
    case 'f':
      confPath = optarg;
      break;
    case 'a':
      addresPath = optarg;
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
    case 'b':
      addBin = optarg;
      break;
    case 'e':
      delBin = optarg;
      break;
    case 'n':
      appName = optarg;
      break;
    case '?':
      printf("Usage: Later\n");
      return 1;
    }
  }

  if(!confPath)
    confPath = strdup(".");

  if(addresPath)
    return addResource(confPath, addresPath, argv[argc-1]);

  //if(delresPath)
  //  return delResource(confPath, delresPath);

  if(setparm)
    return setParam(confPath, setparm, argv[argc-1]);

  //if(unsetparm)
  //  return unsetParm(confPath, setparm);

  if(addBin)
    return setBin(confPath, addBin, argv[argc-1]);

  //if(delBin)
  //  return unsetBin(confPath, delBin);

  if(appName)
    return setName(confPath, appName);

  return 1;
}
