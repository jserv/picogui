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

typedef struct{
  char *workingDir;
  short resourceType;
} resResource;


//-------Launcher related code----------------------------------------
char *resMakeAbsPath(char *rootPath, char *subdir, int appConf);
int resGetAppCount(char *path);
char **resGetAppPaths(char *rootPath);

//-------Resource related code----------------------------------------
resResource *resLoadResource(char *path);
char *resGetProperty(resResource *resource, char *section, char *property);
void *resGetResouce(resResource *resource, char *section, char *property, int *size);
void resUnloadResource(resResource *resource);

