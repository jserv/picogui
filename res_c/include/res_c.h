/* 
 *
 * res_c.h - Resource control (headers)
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

#define RES_APPCONF 0
#define RES_ELF 1

#ifndef u32
typedef unsigned long u32;
#endif

typedef struct{
  void *data;
  u32 size;
} resElement;

typedef struct{
  char *workingDir;
  resElement **resourceTable;
  int resourceCount;
  short resourceType;
} resResource;


//-------Launcher related code----------------------------------------
const char *resMakeAbsPath(const char *rootPath, const char *subdir, int appConf);
int resGetAppCount(const char *path);
const char **resGetAppPaths(const char *rootPath);

//-------Resource related code----------------------------------------
resResource *resLoadResource(const char *path);
void resUnloadResource(resResource *resource);

const char *resGetProperty(resResource *resource, const char *section, const char *property, const char *dparam);
void resSetProperty(resResource *resource, const char *section, const char *property, const char *data);

void *resGetResouce(resResource *resource, const char *section, const char *property, int *size);
const char **resListResources(resResource *resource, const char *section, int *count);

//-------Config related code------------------------------------------
const char *getConfigProperty(resResource *resource, const char *section, const char *property, const char *dparam);
void setConfigProperty(resResource *resource, const char *section, const char *property, const char *data);

