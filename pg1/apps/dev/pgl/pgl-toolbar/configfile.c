/* $Id$
 *
 * configfile.c - Utilities for loading, storing, and retrieving
 *                configuration options
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "configfile.h"

/* Maximum parseable line size in a config file */
#define LINESIZE  256

/******** Data structures */

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

/******** Internal functions */

/* Get a pointer to the named section. If it does not exist return NULL */
struct cfg_section *configfile_getsection(const char *section) {
  static struct cfg_section *cache = NULL;
  struct cfg_section *p;

  /* Cache the last lookup */
  if (sections) {
    if (cache && !strcmp(cache->name,section))
      return cache;
  } else
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
  struct cfg_section *sect;
  
  /* Find the proper section, create one if it doesn't exist */
  sect = configfile_getsection(section);
  if (!sect) {
    /* create the section and section name */
    sect = malloc(sizeof(struct cfg_section) + strlen(section) + 1);
    memset(sect,0,sizeof(struct cfg_section));
    sect->next = sections;
    sections = sect;
    sect->name = ((char*)sect) + sizeof(struct cfg_section);
    strcpy(sect->name,section);
  }
  return sect;
}

g_error configfile_set(struct cfg_section *sect, const char *key,
		       const char *value) {
  struct cfg_item *p;

  /* Find/create the item */
  p = sect->items;
  while (p) {
    if (!strcmp(p->key,key))
      break;
    p = p->next;
  }
  if (!p) {
    /* Create the new item */
    p = malloc(sizeof(struct cfg_item) + strlen(key) + 1);
    //errorcheck;
    memset(p,0,sizeof(struct cfg_item));
    p->next = sect->items;
    sect->items = p;
    p->key = ((char*)p) + sizeof(struct cfg_item);
    strcpy(p->key,key);
  }
  else {
    /* Free the existing value */
    free(p->value);
  }
  
  /* Allocate a new value */
  p->value = malloc(strlen(value)+1);
  //errorcheck;
  strcpy(p->value,value);

  return 1;
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

/******** Public functions */

g_error sub_configfile_parse(const char *filename, struct cfg_section **section) {
  FILE *f;
  char line[LINESIZE];
  char *p,*q;

  f = fopen(filename,"r");
  if (!f)
    return 0;      /* Can't open config file */

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
	return -2; /* Missing ']' */
      *q = 0;
      *section = configfile_makesection(p);
      continue;
    }

    if (!*section)
      return -3;   /* Undefined section */

    /* Want to source another file ? */
    if (p[0]=='.' && (p[1]==' '||p[1]=='\t')) {
      g_error err;
      q = p+2;
      q = strip_head(q);
      strip_tail(q);
      err = sub_configfile_parse(q, section);
    }

    /* If we got this far, it's a key/value line to a defined section */

    /* Get a pointer to the key in p and a pointer to the value in q */
    q = strchr(p,'=');
    if (!q)
      return 0;   /* Missing '=' */
    *q = 0;
    q++;

    /* Cut leading whitespace from q */
    q = strip_head(q);

    /* Chop off trailing whitespace from p and q */
    strip_tail(p);
    strip_tail(q);

    /* Set the parameter! */
    configfile_set(*section,p,q);
  }

  fclose(f);
  return 1;
}

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
	free(p->value);

      condemn = p;
      p = p->next;
      free(condemn);
    }

    condemn = sect;
    sect = sect->next;
    free(condemn);
  }

  sections = NULL;
}

g_error set_param_str(const char *section, const char *key,
		      const char *value) {
  return configfile_set(configfile_makesection(section),key,value);
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

const char **get_section_params(const char *section, int *count){
  struct cfg_section *sect;
  struct cfg_item *p;
  int paramCount = 0;
  char **paramList;

  sect = configfile_getsection(section);
  if (!sect)
    return NULL;
  
  p = sect->items;
  while (p) {
    paramCount++;
    p = p->next;
  }

  if(paramCount > 0){
    
    paramList = malloc(sizeof(char *)*paramCount);
    paramCount = 0;
    
    p = sect->items;
    while (p) {
      paramList[paramCount] = p->value;
      paramCount++;
      p = p->next;
    }
    
    *count = paramCount;
    return paramList;
  }else{
    *count = 0;
    return NULL;
  }
}

g_error configfile_write(const char *filename){
  FILE *outputFile = NULL;
  struct cfg_section *thisSection;
  struct cfg_item *thisItem;

  if(sections){
    thisSection = &sections[0];
    if((outputFile = fopen(filename, "w"))){
      while(thisSection){
	fprintf(outputFile, "[%s]\n", thisSection->name);
	thisItem = &thisSection->items[0];
	while(thisItem){
	  fprintf(outputFile, "%s=%s\n", thisItem->key, thisItem->value);
	  thisItem = thisItem->next;
	}
	thisSection = thisSection->next;
      }
      fclose(outputFile);
      return 1;
    }else{
      return 0;
    }
  }else{
    return 0;
  }
}

/* The End */



