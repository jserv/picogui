/* $Id$
 *
 * platforms.c - Contains platform-dependant stuff
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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
 *   Philippe Ney <philippe.ney@smartdata.ch> :
 *   initial version
 *
 *   Pascal Bauermeister <pascal.bauermeister@smartdata.ch>
 * 
 */



#include <stdio.h>
#include <stdarg.h>
#include <string.h>



/* Define G_VA_COPY() to do the right thing for copying va_list variables.
 * va_list is a pointer.
 * From code in 'glib.h'
 */
#define G_VA_COPY(ap1, ap2)     ((ap1) = (ap2))


/* Return the size of the list
 * Code derived from the on in the 'glib'
 */
int my_g_printf_string_upper_bound (const char* format, va_list args) {
  int len = 1;

  while (*format) {
    int long_int   = 0;
    int extra_long = 0;
    char c;

    c = *format++;
    if (c == '%') {
      int done = 0;

      while (*format && !done) {
        switch (*format++) {
          char *string_arg;

          case '*':
            len += va_arg (args, int);
            break;
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            /* add specified format length, since it might exceed the
             * size we assume it to have.
             */
            format -= 1;
            len += strtol (format, (char**) &format, 10);
            break;
          case 'h':
            /* ignore short int flag, since all args have at least the
             * same size as an int
             */
            break;
          case 'l':
            if (long_int)
              extra_long = 1; /* linux specific */
            else
              long_int = 1;
            break;
          case 'q':
          case 'L':
            long_int = 1;
            extra_long = 1;
            break;
          case 's':
            string_arg = va_arg (args, char *);
            if (string_arg)
              len += strlen (string_arg);
            else {
              /* add enough padding to hold "(null)" identifier */
              len += 16;
            }
            done = 1;
            break;
          case 'd':
          case 'i':
          case 'o':
          case 'u':
          case 'x':
          case 'X':
            if (long_int)
              (void) va_arg (args, long);
            else
              (void) va_arg (args, int);
            len += extra_long ? 64 : 32;
            done = 1;
            break;
          case 'D':
          case 'O':
          case 'U':
            (void) va_arg (args, long);
            len += 32;
            done = 1;
            break;
          case 'e':
          case 'E':
          case 'f':
          case 'g':
              (void) va_arg (args, double);
            len += extra_long ? 128 : 64;
            done = 1;
            break;
          case 'c':
            (void) va_arg (args, int);
            len += 1;
            done = 1;
            break;
          case 'p':
          case 'n':
            (void) va_arg (args, void*);
            len += 32;
            done = 1;
            break;
          case '%':
            len += 1;
            done = 1;
            break;
          default:
            /* ignore unknow/invalid flags */
            break;
        }
      }
    }
    else
      len += 1;
  }
  return len;
}

/* By now just emulate a vsnprintf function.
 * But integrate it directly in '_pg_dynformat' of picogui_client.c could
 * maybe make this last more efficient.
 */
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args) {
  va_list args2;

  G_VA_COPY (args2, args);

  /* if the size of the args to write is bigger than 'size' return -1 */
  if((my_g_printf_string_upper_bound (fmt,args)) > size)
    return -1;
    
  return (vsprintf(buf,fmt,args2));
}

/* Kludge? */
