#ifndef _UTIL_H
#define _UTIL_H

/* Strip trailing newline (UNIX or MSDOS) off of a string. */
void strip_newline(char * str);

/* Parse a line from a standard CSV file and return it in argv-style array. */
int csv_to_array(char *str, char *buf, char **array, int *count);

/* Parse a line from an extended CSV file and return it in argv-style array. */
int str_to_array(char *str, char *buf, char *delim, int multiple_delim,
		 char **array, int *count_ptr);

/* ** Tasnim Ahmed <tasnim@users.sourceforge.net> ** */

/* get estimated rows and columns from a file */
int csv_info ( char *fileName, int *cols, int *rows );

/* Parse a whole file */
int parse_csv ( char *fileName, char **target, int cols, int rows );

#endif
