/*
 * String Parsing Utility Functions
 * Copyright (C) 1999 by Tom Dyas (tdyas@vger.rutgers.edu)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "util.h"

void
strip_newline(char * str)
{
    char * p;

    p = str + strlen(str) - 1;
    while (p >= str && (*p == '\r' || *p == '\n'))
	*p-- = '\0';
}

int
csv_to_array(char * str, char * buf, char **array, int * count_ptr)
{
    int count, in_quotes, saw_delim;

    count = 0;
    while (1) {
	/* Determine if we need to start in quote mode. */
	if (*str == '"') {
	    in_quotes = 1;
	    str++;
	} else {
	    in_quotes = 0;
	}

	/* Mark the start of the field in the output buffer. */
	array[count] = buf;
	count++;
	saw_delim = 0;

	/* Loop while there are still characters remaining. */
	while (*str) {
	    switch (*str) {
	    case ',':
		if (in_quotes) {
		    *buf++ = *str++;
		} else {
		    str++;
		    saw_delim = 1;
		    goto end_loop; /* okay since used as a 2-level 'break' */
		}
		break;

	    case '"':
		if (in_quotes && str[1] == '"') {
		    *buf++ = '"';
		    str += 2;
		} else {
		    in_quotes = (! in_quotes);
		    str++;
		}
		break;

	    default:
		*buf++ = *str++;
		break;
	    }
	}
    end_loop:

	/* Make sure that we didn't end in quote mode. */
	if (in_quotes) return -1;

	/* Mark the end of this field by adding null byte. */
	*buf++ = '\0';

	/* Bail out if we hit the end of the string. */
	if (*str == '\0' && !saw_delim)
	    break;
    }

    /* Terminate the array and return count of items. */
    array[count] = NULL;
    *count_ptr = count;

    return 0;
}

int
str_to_array(char * str, char * buf, char * delim, int multiple_delim,
	     char **array, int * count_ptr)
{
    int state, count, num, saw_delim;
    char *d, numstr[16];

#define STATE_NORMAL                0
#define STATE_QUOTE_DOUBLE          1
#define STATE_QUOTE_SINGLE          2
#define STATE_BACKSLASH             3
#define STATE_BACKSLASH_DOUBLEQUOTE 4

    count = 0;

    while (1) {
	/* Determine what our initial state will be. */
	switch (*str) {
	case '"':
	    state = STATE_QUOTE_DOUBLE;
	    str++;
	    break;
	case '\'':
	    state = STATE_QUOTE_SINGLE;
	    str++;
	    break;
	default:
	    state = STATE_NORMAL;
	    break;
	}

	/* Point the argv-array at the start of this field. */
	array[count] = buf;
	count++;
	saw_delim = 0;

	/* Begin copying data to the output buffer. */
	while (*str) {
	    switch (state) {
	    case STATE_NORMAL:
		if (strchr(delim, *str) != NULL) {
		    if (multiple_delim) {
			while (*str && strchr(delim, *str) != NULL)
			    str++;
		    } else
			str++;
		    saw_delim = 1;
		    goto end_loop;
		} else if (*str == '"')
		    state = STATE_QUOTE_DOUBLE;
		else if (*str == '\'')
		    state = STATE_QUOTE_SINGLE;
		else if (*str == '\\')
		    state = STATE_BACKSLASH;
		else
		    *buf++ = *str;
		break;

	    case STATE_QUOTE_DOUBLE:
		if (*str == '"')
		    state = STATE_NORMAL;
		else if (*str == '\\')
		    state = STATE_BACKSLASH_DOUBLEQUOTE;
		else
		    *buf++ = *str;
		break;

	    case STATE_QUOTE_SINGLE:
		if (*str == '\'')
		    state = STATE_NORMAL;
		else
		    *buf++ = *str;
		break;

	    case STATE_BACKSLASH:
		*buf++ = *str;
		state = STATE_NORMAL;
		break;

	    case STATE_BACKSLASH_DOUBLEQUOTE:
		switch (*str) {
		case '\\':
		    *buf++ = '\\';
		    break;
		case 'n':
		    *buf++ = '\n';
		    break;
		case 'r':
		    *buf++ = '\r';
		    break;
		case 't':
		    *buf++ = '\t';
		    break;
		case '"':
		    *buf++ = '"';
		    break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		    d = str;
		    while (*d && (*d >= '0' && *d <= '9')) d++;
		    memset(numstr, 0, sizeof(numstr));
		    memcpy(numstr, str, d - str);
		    num = atoi(numstr);
		    if (num < 0 || num > 255)
			return -1;
		    *buf++ = (char) (num & 0xFF);
		    break;
		}
		state = STATE_QUOTE_DOUBLE;
		break;
	    }

	    /* Advance the pointer to the next character. */
	    str++;
	}
    end_loop:

	/* Make sure that we didn't end out of normal mode. */
	if (state != STATE_NORMAL) return -1;

	/* Mark the end of this field by adding null byte. */
	*buf++ = '\0';

	/* Bail out if we hit the end of the string. */
	if (*str == '\0' && !saw_delim)
	    break;

    }

    /* Terminate the array and return count of items. */
    array[count] = NULL;
    *count_ptr = count;

    return 0;
}

int csv_info ( char *fileName, int *cols, int *rows )
{
	char input[1024], temp[1024];
	char *data[50];
	int c=0, r=0, i;
   FILE *csv;

//quick and dirty hack to count lines
	if( ( csv = fopen ( fileName, "rt" ) ) == NULL)
	{
		fprintf(stderr, "Cannot open input file.\n");
		exit(1);
	}
	while( !feof ( csv ) )
	{
	   if ( fgets ( input, sizeof ( input ), csv ) == NULL )
			break;
		if ( c==0 )
		{
			strip_newline ( input );
			str_to_array ( input, temp, ",", 0, data, &i );
			c = i;
		}
		r++;
	}
	fclose ( csv );

	*cols = c ;
	*rows = r ;

}


int parse_csv ( char *fileName, char **target, int cols, int rows )
{
	char input[1024], temp[1024];
	int i, k, r=0;
	FILE *csv;
	char **data;

	data = (char **) malloc ( cols );
	for ( k = 0; k < cols; k++ )
      data[k] = malloc ( 1024 );

	if( ( csv = fopen ( fileName, "rt" ) ) == NULL)
	{
		fprintf(stderr, "Cannot open input file.\n");
		return -1;
	}

	while( !feof ( csv ) )
	{
		if ( fgets ( input, sizeof ( input ), csv ) == NULL )
			break;
		strip_newline ( input );
		str_to_array ( input, temp, ",", 0, data, &i );

		for ( k = 0; k < cols; k++ )
		{
			target [ (r*cols) + k] = malloc ( 1024 );
			strcpy ( target [ (r*cols) + k], data[k] );
		}
      r++;
	}
	fclose ( csv );

	return 0;
}

