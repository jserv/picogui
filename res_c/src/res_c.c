/* 
 *
 * res_c.c - Resource control 
 *
 *
 * libres resource control system
 * Copyright (C) 2000-2002 Daniel Jackson <carpman@voidptr.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors: 
 * 
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "res_c.h"
#include "confparse.h"

//-------Launcher related code----------------------------------------
char *resMakeAbsPath(char *rootPath, char *subdir, int appConf){
  char *appPath = NULL;

  if(!appConf){
    appPath = malloc(strlen(rootPath)+strlen(subdir)+3);
    sprintf(appPath, "%s/%s/", rootPath, subdir);
  }else{
    appPath = malloc(strlen(rootPath)+strlen(subdir)+strlen("/app.conf")+3);
    sprintf(appPath, "%s/%s/app.conf", rootPath, subdir);
  }
  return appPath;
}

int resGetAppCount(char *path){
  DIR *directory = opendir(path);
  struct dirent *dinfo = NULL;
  FILE *testOpen;
  char *appPath = NULL;
  int appCount = 0;

  if(directory){
    while(dinfo = readdir(directory)){
      if(0){
	//ELF check code goes here
      }else{
	appPath = resMakeAbsPath(path, dinfo->d_name, 1);
	if(testOpen = fopen(appPath, "r")){
	  appCount++;
	  fclose(testOpen);
	}
	free(appPath);
      }
    }
  }
  return appCount;
}
   
char **resGetAppPaths(char *rootPath){
  DIR *directory = opendir(rootPath);
  struct dirent *dinfo = NULL;
  char *appPath = NULL;
  char **output = NULL;
  int appItr = 0, appCount = 0;
  FILE *testOpen = NULL;

  if(directory){
    appCount = resGetAppCount(rootPath);
    output = malloc(sizeof(char *)*resGetAppCount(rootPath));
    while(dinfo = readdir(directory)){
      if(0){
	//ELF check code goes here
      }else{
	appPath = resMakeAbsPath(rootPath, dinfo->d_name, 1);
	if(testOpen = fopen(appPath, "r")){
	  output[appItr] = resMakeAbsPath(rootPath, dinfo->d_name, 0);
	  appItr++;
	  fclose(testOpen);
	}
      }
    }
  }
  return output;
}

//-------Resource related code----------------------------------------
resResource *resLoadResource(char *path){
  resResource *newResource = malloc(sizeof(resResource));

  //This needs to do checking to see if we load app.conf or a resource.
  //BTW: My feeling is load ELF if an app.conf is not found. That way, 
  //the settings in the ELF can be overridden with an app.conf. (CTG)
  
  if(path){
    newResource->workingDir = strdup(path);
  }else{
    newResource->workingDir = getcwd(newResource->workingDir, 0);
  }
  newResource->resourceType = RES_APPCONF;
  return newResource;
}

char *resGetProperty(resResource *resource, char *section, char *property){
  switch(resource->resourceType){
  case RES_ELF:
    //Do some interesting stuff here.
    break;
  case RES_APPCONF:
    return resGetACProperty(resource, section, property);
    break;
  default:
    break;
  }
  return NULL;
}

void *resGetResource(resResource *resource, char *section, char *property, int *size){
  void *propertyData = NULL;
  char *pathName;
  struct stat st;
  int fd;

  switch(resource->resourceType){
  case RES_ELF:
    //Do some interesting stuff here.
    break;
  case RES_APPCONF:
    if(pathName = resGetACProperty(resource, section, property)){
      fd = open(pathName, O_RDONLY);
      fstat(fd, &st);
      propertyData = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
      if(size)
	*size = st.st_size;
      close(fd);
      free(pathName);
    }
    break;
  default:
    break;
  }
  return propertyData;
}

void resUnloadResource(resResource *resource){
  if(resource){
    if(resource->workingDir)
      free(resource->workingDir);
    free(resource);
  }
}