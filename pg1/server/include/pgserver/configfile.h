/* $Id$
 *
 * pgserver/configfile.h - Header for pgserver's configuration database
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

#ifndef __H_CONFIGFILE
#define __H_CONFIGFILE

/* Parse the given config file and add its contents to the config database */
g_error configfile_parse(const char *filename);

/* Parse config files found in the default locations */
g_error configfile_parse_default(void);

/* Free the config database containing data from everything read so far */
void configfile_free(void);

/* Add or modify a value in the config database */
g_error set_param_str(const char *section, const char *key, const char *value);

/* If the key exists, append separator and value, if not set it to value */
g_error append_param_str(const char *section, const char *key, 
			 const char *separator, const char *value);

/* Retrieve a value formatted as an integer/string, or if it doesn't
 * exist return the supplied default 
 */
int get_param_int(const char *section, const char *key, int def); 
const char *get_param_str(const char *section, const char* key, const char *def); 

/* Parse a command line, and add it to the config file database.
 * If the command line was invalid or the user requested help, a help string
 * will be printed and an error will be returned.
 */
g_error commandline_parse(int argc, char **argv);

#endif /* __H_CONFIGFILE */
/* The End */



