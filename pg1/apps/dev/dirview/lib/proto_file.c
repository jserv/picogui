/*
   Copyright (C) 2002 by Pascal Bauermeister

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with PocketBee; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Pascal Bauermeister
   Contributors:

   $Id$
*/

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "libpg_dirview.h"
#include "protocol.h"
#include "debug.h"

#define FREE(var)          { if(var) free((void*)(var)); var=0; }
#define REASSIGN(var, val) { FREE(var); var=val; }

/*****************************************************************************/
/* Protos */

static void* file_protocol_init(void);
static void  file_protocol_uninit(void*);

static int file_site_enter(void*, const char* site_name, int port,
			   const char* username, const char* password);
static int file_site_leave(void*, const char* site_name, int port);

static int file_dir_enter(void*, const char* dir_name);
static int file_dir_leave(void*, const char* dir_name);

static int file_item_next(void*,
			  int row_nr, void** ref,
			  LpgdvProperties* prop);
static int file_item_load(void* pdata,
			  const char* full_path,
			  const char* item_name,
			  const char* local_name,
			  int show_progress);

static const char* file_getdirname(const char* path);
static const char* file_getitemname(const char* path);

static int file_is_same_site(const char*, const char*);
static int file_is_same_dir(const char*, const char*);

/*****************************************************************************/
/* Functions table */

const LpgdvProtocol file_protocol = {
  "file",
  ( LPGDV_PROP_TYPE |
    LPGDV_PROP_NAME |
    LPGDV_PROP_PROTECTION |
    LPGDV_PROP_SIZE |
    LPGDV_PROP_OWNER |
    LPGDV_PROP_GROUP |
    LPGDV_PROP_CRE_DATE |
    LPGDV_PROP_MOD_DATE |
    LPGDV_PROP_ACC_DATE |
    LPGDV_PROP_MIME_TYPE
    ),
  0, /* has_login */
  0, /* has_host */
  1, /* has_arguments */
  0, /* default_port */
  file_protocol_init,
  file_protocol_uninit,
  file_site_enter,
  file_site_leave,
  file_dir_enter,
  file_dir_leave,
  file_item_next,
  file_item_load,
  file_getdirname,
  file_getitemname,
  file_is_same_site,
  file_is_same_dir,
};

/*****************************************************************************/

/* This is the size of the buffer files are returned in */
#define FILEMAX 512

/* This is the maximum size of an individual file name */
#define NAMEMAX 80

typedef struct {
  char name[NAMEMAX];
  struct stat st;
} Filenode;

/* Our pdata (instance) structure */
typedef struct {
  const char*      dir_name;
  struct dirent*   dirent;
  struct stat      stat;
  Filenode*        nodes;
  char dir_buf[FILEMAX];
  char path_buf[FILEMAX];
} Instance;

/*****************************************************************************/

static void
fullpath(const char *file, const char* dir, char* path)
{
  int len = strlen(dir);
  strcpy(path, dir);
  if (len<(FILEMAX-1) && path[len-1]!='/') {
    strcat(path, "/");
    len--;
  }
  strncat(path, file, FILEMAX-1-len);
}

static int
filter(unsigned int flags, const char *name, struct stat *st)
{
  return 1;
}


/*****************************************************************************/

static void*
file_protocol_init(void)
{
  ENTER("file_protocol_init()");
  Instance* i = malloc(sizeof(Instance));
  LEAVE;
  return i;
}

static void
file_protocol_uninit(void* pdata)
{
  ENTER("file_protocol_uninit()");
  Instance* inst = (Instance*)pdata;

  if(inst) {
    FREE(inst->dir_name);
    FREE(inst);
  }

  LEAVE;
  return;
}

static int
file_site_enter(void* pdata, const char* site_name, int port,
		const char* username, const char* password)
{
  ENTER("file_site_enter()");
  LEAVE;
  return 1;
}

static int
file_site_leave(void* pdata, const char* site_name, int port)
{
  ENTER("file_site_leave()");
  LEAVE;
  return ;
}

static int
file_dir_enter(void* pdata, const char* dir_name)
{
  ENTER("file_dir_enter()");
  Instance* inst = (Instance*)pdata;
  int ret = 0;

  if(inst) {
    DIR *dir;
    struct dirent* de;
    Filenode* fn;
    int nb_avail = 0;
    int nb_taken = 0;

    memset(inst, 0, sizeof(Instance));
    inst->dir_name = strdup(dir_name);

    /* first just count the files */
    dir = opendir(dir_name);
    if (dir==0) goto done;
    while (readdir(dir))
      ++nb_avail;
    rewinddir(dir);

    /* alloc mem for the nodes */
    inst->nodes = malloc(nb_avail * sizeof(Filenode));
    if(inst->nodes==0) {
      closedir(dir);
      goto done;
    }

    /**/
    fn = inst->nodes;
    while (nb_taken<nb_avail && (de = readdir(dir))) {
      struct stat st;
      fullpath(de->d_name, dir_name, inst->path_buf);
      lstat(inst->path_buf, &st);
      if(filter(0, de->d_name, &st)) {
	memcpy(&fn->st, &st, sizeof(st));
	fn->name[NAMEMAX-1] = 0;
	strncpy(fn->name, de->d_name, NAMEMAX-1);
	DPRINTF("++ [%s]\n", fn->name);
	++fn;
	++nb_taken;
      }
    }
    closedir(dir);
  }

 done:
  LEAVE;
  return ret;
}

static int
file_dir_leave(void* pdata, const char* dir_name)
{
  ENTER("file_dir_leave()");
  Instance* inst = (Instance*)pdata;

  if(inst) {
    FREE(inst->dir_name);
  }

  LEAVE;
  return 1;
}

static int
file_item_next(void* pdata, int row_nr, void** ref, LpgdvProperties* prop)
{
  ENTER("file_item_next()");
  LEAVE;
  return 0;
}

static int
file_item_load(void* pdata,
	       const char* full_path,
	       const char* item_name,
	       const char* local_name,
	       int show_progress)
{
  return -1;
}

static const char*
file_getdirname(const char* path)
{
  ENTER("file_getdir()");
  char* dir = 0;
  if(path && strlen(path) && (dir=strdup(path)) ) {
    char* p = strrchr(dir, '/'); /* unix ! */
    if(p) *++p = '\0';
  }
  LEAVE;
  return dir;
}

static const char*
file_getitemname(const char* path)
{
  ENTER("file_getdir()");
  char* file = 0;
  if(path && strlen(path)) {
    char* p = strrchr(path, '/'); /* unix ! */
    if(p) file = strdup(p+1);
  }
  LEAVE;
  return file;
}

static int
file_is_same_site(const char* name1, const char* name2)
{
  /* site not supported: always match ! */
  return 1;
}

static int
file_is_same_dir(const char* name1, const char* name2)
{
  if(name1==0 && name2==0) return 0;
  if(name1==0) return 0;
  if(name2==0) return 0;
  return !strcmp(name1, name2);
}
