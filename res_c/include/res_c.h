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
 * License along with this li/* 
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
char *resMakeAbsPath(char *rootPath, char *subdir, int appConf);
int resGetAppCount(char *path);
char **resGetAppPaths(char *rootPath);

//-------Resource related code----------------------------------------
resResource *resLoadResource(char *path);
char *resGetProperty(resResource *resource, char *section, char *property);
void resSetProperty(resResource *resource, char *section, char *property, char *data);
void *resGetResouce(resResource *resource, char *section, char *property, int *size);
void resUnloadResource(resResource *resource);


//#ifndef PGMEMDAT_NEED_FREE

//typedef long u32;

//struct pgmemdata {
//  void *pointer;       //!< when null, indicates error
//  u32 size;  //!< size in bytes of data block
//  int flags;           //!< PGMEMDAT_* flags or'ed together
//};
//#define PGMEMDAT_NEED_FREE    0x0001   //!< pgmemdata should be free()'d when done
//#define PGMEMDAT_NEED_UNMAP   0x0002   //!< pgmemdata should be munmap()'d when done

//------------PicoGUI specific functions-----------------------
//struct pgmemdata pgFromResource(resResource *resource, char *resourceName);

//#endif
