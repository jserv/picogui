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
const char *resMakeAbsPath(const char *rootPath, const char *subdir, int appConf){
  char *appPath = NULL;

  if(!appConf){
    appPath = malloc(strlen(rootPath)+strlen(subdir)+2);
    sprintf(appPath, "%s/%s", rootPath, subdir);
  }else{
    appPath = malloc(strlen(rootPath)+strlen(subdir)+strlen("/app.conf")+3);
    sprintf(appPath, "%s/%s/app.conf", rootPath, subdir);
  }
  return appPath;
}

int resGetAppCount(const char *path){
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
   
const char **resGetAppPaths(const char *rootPath){
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
resResource *resLoadResource(const char *path){
  resResource *newResource = malloc(sizeof(resResource));

  //This needs to do checking to see if we load app.conf or a resource.
  //BTW: My feeling is load ELF if an app.conf is not found. That way, 
  //the settings in the ELF can be overridden with an app.conf. (CTG)
  
  if(path){
    newResource->workingDir = strdup(path);
  }else{
    newResource->workingDir = getcwd(newResource->workingDir, 0);
  }
  newResource->resourceTable = malloc(sizeof(resElement *));
  newResource->resourceType = RES_APPCONF;
  return newResource;
}

const char *resGetProperty(resResource *resource, const char *section, const char *property, const char *dparam){
  switch(resource->resourceType){
  case RES_ELF:
    //Do some interesting stuff here.
    break;
  case RES_APPCONF:
    return (char *)resGetACProperty(resource, section, property, dparam);
    break;
  default:
    break;
  }
  return dparam;
}

void resSetProperty(resResource *resource, const char *section, const char *property, const char *data){
  switch(resource->resourceType){
  case RES_ELF:
    //Do some REALY interesting stuff here.
    break;
  case RES_APPCONF:
    resSetACProperty(resource, section, property, data);
    break;
  default:
    break;
  }
}

void *resGetResource(resResource *resource, const char *section, const char *property, int *size){
  void *propertyData = NULL;
  char *pathName;
  struct stat st;
  int fd;

  switch(resource->resourceType){
  case RES_ELF:
    //Do some interesting stuff here.
    break;
  case RES_APPCONF:
    if(pathName = (char *)resGetACProperty(resource, section, property, NULL)){
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
  if(propertyData){
    resource->resourceCount++;
    resource->resourceTable = realloc(resource->resourceTable, sizeof(resElement *)*resource->resourceCount+1);
    resource->resourceTable[resource->resourceCount] = malloc(sizeof(resElement));
    resource->resourceTable[resource->resourceCount]->data = propertyData;
    resource->resourceTable[resource->resourceCount]->size = *size;
    printf("New resource %x\n", resource->resourceTable[resource->resourceCount]->data);
  }
  return propertyData;
}

const char **resListResources(resResource *resource, const char *section, int *count){
  
 switch(resource->resourceType){
 case RES_ELF:
   break;
 case RES_APPCONF:
   return resListACProperties(resource, section, count);
   break;
 default:
   break;
 }
 
 return NULL;
}

void resUnloadResource(resResource *resource){
  if(resource){
    for(;resource->resourceCount > 0; resource->resourceCount--){
      printf("Freeing resource %x\n", resource->resourceTable[resource->resourceCount-1]->data);
      munmap(resource->resourceTable[resource->resourceCount-1]->data, 
	     resource->resourceTable[resource->resourceCount-1]->size);
    }
    free(resource->resourceTable);
    if(resource->workingDir)
      free(resource->workingDir);
    free(resource);
  }
}


//-------Config related code-----------------------------------------
const char *getConfigProperty(resResource *resource, const char *section, const char *property, const char *dparam){
  char *configPath;
  char *property;
  resResource confResource;
  
  if(configPath = resGetProperty(resource, "Config", "path", NULL)){
    confResource.workingDir = configPath;
    property = resGetACProperty(&confResource, section, property, dparam);
    free(configPath);
    return property;
  }
  return dparam;
}

void setConfigProperty(resResource *resource, const char *section, const char *property, const char *data){
  char *configPath;
  resResource confResource;

  if(configPath = resGetProperty(resource, "Config", "path", NULL)){
    confResource.workingDir = configPath;
    resSetACProperty(&confResource, section, property, data);
    free(configPath);
  }
}
