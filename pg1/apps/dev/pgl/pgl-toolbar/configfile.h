/* $Id$
 *
 * pgserver/configfile.h - Header for the configuration file utilities
 *                         in configfile.c
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

#ifndef __H_CONFIGFILE
#define __H_CONFIGFILE

typedef short g_error;

g_error configfile_parse(const char *filename);
void configfile_free(void);

g_error set_param_str(const char *section, const char *key, const char *value);

int get_param_int(const char *section, const char *key, int def); 
const char *get_param_str(const char *section, const char* key, 
			  const char *def); 

const char **get_section_params(const char *section, int *count);

g_error configfile_write(const char *filename);

#endif /* __H_CONFIGFILE */

/* The End */



