/* 
 *
 * confparse.h - app.conf file parsing routines (header)
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

int configfile_parse(const char *filename);
void configfile_free(void);

int set_param_str(const char *section, const char *key, const char *value);

int get_param_int(const char *section, const char *key, int def); 
char *get_param_str(const char *section, const char* key, 
			  const char *def); 

char **get_section_params(const char *section, int *count);

int configfile_write(const char *filename);

//---------Config parsing--------------------------------
char *resCGetACProperty(resResource *resource, char *section, char *property);
