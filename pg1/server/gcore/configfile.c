/* $Id$
 *
 * configfile.c - Utilities for loading, storing, and retrieving
 *                configuration options
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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
 * 
 */

#include <pgserver/common.h>
#include <pgserver/configfile.h>

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Maximum parseable line size in a config file */
#define LINESIZE  256

/* The config options are stored as a linked list of sections,
 * each section with a linked list of key-value pairs.
 */
struct cfg_item {
  char *key;              /* Does not need to be freed */
  char *value;            /* Does need to be freed */
  struct cfg_item *next;
};
struct cfg_section {
  char *name;             /* Does not need to be freed */
  struct cfg_item *items;
  struct cfg_section *next;
};

struct cfg_section *sections;

struct cfg_section *configfile_getsection(const char *section);
struct cfg_section *configfile_makesection(const char *section);
g_error configfile_set(struct cfg_section *sect, const char *key, const char *value);
char *strip_head(char *s);
void strip_tail(char *s);
g_error sub_configfile_parse(const char *filename, struct cfg_section **section);
g_error configfile_parse_if_exists(const char *filename);


/******************************************************** Public functions **/

g_error configfile_parse(const char *filename) {
  struct cfg_section *section = NULL;
  return sub_configfile_parse(filename, &section);
}

void configfile_free(void) {
  struct cfg_section *sect;
  struct cfg_item *p;
  void *condemn;

  sect = sections;
  while (sect) {

    p = sect->items;
    while (p) {
      if (p->value)
	g_free(p->value);

      condemn = p;
      p = p->next;
      g_free(condemn);
    }

    condemn = sect;
    sect = sect->next;
    g_free(condemn);
  }

  sections = NULL;
}

g_error set_param_str(const char *section, const char *key,
		      const char *value) {
  return configfile_set(configfile_makesection(section),key,value);
}

/* If the key exists, append separator and value, if not set it to value */
g_error append_param_str(const char *section, const char *key, 
			 const char *separator, const char *value) {
  char *newvalue;
  const char *oldvalue;
  
  oldvalue = get_param_str(section,key,NULL);

  if (oldvalue) {
    /* Append to an existing value */ 
    newvalue = alloca(strlen(oldvalue)+strlen(separator)+strlen(value)+1);
    strcpy(newvalue,oldvalue);
    strcat(newvalue,separator);
    strcat(newvalue,value);
    return set_param_str(section,key,newvalue);
  }
    
  return set_param_str(section,key,value);
}

int get_param_int(const char *section, const char* key, int def) {
  const char *strval;
  strval = get_param_str(section,key,NULL);
  if (strval)
    return atoi(strval);
  return def;
}

const char *get_param_str(const char *section, const char* key, 
			  const char *def) {
  struct cfg_section *sect;
  struct cfg_item *p;

  sect = configfile_getsection(section);
  if (!sect)
    return def;
  
  p = sect->items;
  while (p) {
    if (!strcmp(p->key,key))
      return p->value;
    p = p->next;
  }
  return def;
}

/* Parse config files found in the default locations */
g_error configfile_parse_default(void) {
  g_error e;
  char *rcpath;
  const char *home = getenv("HOME");
  const char *rcname = "/.pgserverrc";

  e = configfile_parse_if_exists("/etc/pgserver.conf");
  errorcheck;

  /* ~/.pgserverrc */
  if (home) {
    rcpath = alloca(strlen(home)+strlen(rcname)+1);
    strcpy(rcpath,home);
    strcat(rcpath,rcname);
    e = configfile_parse_if_exists(rcpath);
    errorcheck;
  }

  return success;
}


/******************************************************** Internal utilities **/

/* Get a pointer to the named section. If it does not exist return NULL */
struct cfg_section *configfile_getsection(const char *section) {
  static struct cfg_section *cache = NULL;
  struct cfg_section *p;

  /* Cache the last lookup */
  if (sections) {
    if (cache && !strcmp(cache->name,section))
      return cache;
  }
  else
    cache = NULL;

  p = sections;
  while (p) {
    if (!strcmp(p->name,section)) {
      cache = p;
      return p;
    }
    p = p->next;
  }
  return NULL;
}

/* Get the section, creating it if it doesn't exist */
struct cfg_section *configfile_makesection(const char *section) {
  size_t len;
  struct cfg_section *sect;
  g_error e;
  
  /* Find the proper section, create one if it doesn't exist */
  sect = configfile_getsection(section);
  if (!sect) {
    len=strlen(section)+1;
    /* create the section and section name */
    e = g_malloc((void**) &sect,
		 sizeof(struct cfg_section) + len);
    if (iserror(e))
      return NULL;
    memset(sect,0,sizeof(struct cfg_section));
    sect->next = sections;
    sections = sect;
    sect->name = ((char*)sect) + sizeof(struct cfg_section);
    memcpy(sect->name,section,len);
  }
  return sect;
}

g_error configfile_set(struct cfg_section *sect, const char *key,
		       const char *value) {
  struct cfg_item *p;
  g_error e;
  size_t len;

  /* Find/create the item */
  p = sect->items;
  while (p) {
    if (!strcmp(p->key,key))
      break;
    p = p->next;
  }
  if (!p) {
    len=strlen(key)+1;
    /* Create the new item */
    e = g_malloc((void**) &p,
		 sizeof(struct cfg_item) + len);
    errorcheck;
    memset(p,0,sizeof(struct cfg_item));
    p->next = sect->items;
    sect->items = p;
    p->key = ((char*)p) + sizeof(struct cfg_item);
    memcpy(p->key,key,len);
  }
  else {
    /* Free the existing value */
    g_free(p->value);
  }
  
  /* Allocate a new value */
  len=strlen(value)+1;
  e = g_malloc((void**) &p->value,len);
  errorcheck;
  memcpy(p->value,value,len);

  return success;
}

/* little utilities to strip whitespace from the head/tail of a string */

char *strip_head(char *s) {
  while (*s && isspace(*s))
    s++;
  return s;
}  

void strip_tail(char *s) {
  char *p;
  p = s;
  while (*p)
    p++;
  p--;
  while (p>=s) {
    if (!isspace(*p))
      break;
    *p = 0;
    p--;
  }
}

g_error sub_configfile_parse(const char *filename, struct cfg_section **section) {
  FILE *f;
  char line[LINESIZE];
  char *p,*q;

  f = fopen(filename,"r");
  if (!f)
    return mkerror(PG_ERRT_IO,37);      /* Can't open config file */

  while (fgets(line,LINESIZE,f)) {
    p = line;

    /* Chomp up all the leading whitespace */
    p = strip_head(p);
    
    /* Skip blank lines and comments */
    if ((!*p) || *p=='#')
      continue;

    /* Process section lines */
    if (*p=='[') {
      /* Skip the open bracket */
      p++;
      /* Stick a null in place of the close bracket */
      q = strchr(p,']');
      if (!q)
	return mkerror(PG_ERRT_BADPARAM,38); /* Missing ']' */
      *q = 0;
      *section = configfile_makesection(p);
      continue;
    }

    if (!*section)
      return mkerror(PG_ERRT_BADPARAM,39);   /* Undefined section */

    /* Want to source another file ? */
    if (p[0]=='.' && (p[1]==' '||p[1]=='\t')) {
      g_error err;
      q = p+2;
      q = strip_head(q);
      strip_tail(q);
      err = sub_configfile_parse(q, section);
      if (iserror(err))
	return err;
      else continue;
    }

    /* If we got this far, it's a key/value line to a defined section */

    /* Get a pointer to the key in p and a pointer to the value in q */
    q = strchr(p,'=');
    if (!q) {
      /* Missing "=", assume the value is boolean and it's being defined as 1 */
      q = "1";
    }
    else {
      *q = 0;
      q++;
    }

    /* Cut leading whitespace from q */
    q = strip_head(q);

    /* Chop off trailing whitespace from p and q */
    strip_tail(p);
    strip_tail(q);

    /* Set the parameter! */
    configfile_set(*section,p,q);
  }

  fclose(f);
  return success;
}

/* Try to parse a config file if it exists */
g_error configfile_parse_if_exists(const char *filename) {
  FILE *f;
  
  f = fopen(filename,"r");
  if (f) {
    fclose(f);
    return configfile_parse(filename);
  }

  return success;
}

/* The End */



