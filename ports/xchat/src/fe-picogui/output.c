/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/fe.h"

#include <picogui.h>
#include "fe-picogui.h"


static int
get_stamp_str (time_t tim, char *dest, int size)
{
	return strftime (dest, size, prefs.stamp_format, localtime (&tim));
}

static int
timecat (char *buf)
{
	char stampbuf[64];

	get_stamp_str (time (0), stampbuf, sizeof (stampbuf));
	strcat (buf, stampbuf);
	return strlen (stampbuf);
}

static int colconv_rgb[] = { 0xcfcfcf, 0x000000, 0x0000cc, 0x00cc00, 0xdd0000,
	0xaa0000, 0xbb00bb, 0xffaa00, 0xeedd22, 0x33de55, 0x00cccc, 0x33ddee,
	0x0000ff, 0xee22ee, 0x777777, 0x999999 };

static void
fe_print_text_textbox (struct session *sess, char *text)
{
	int dotime = FALSE;
	char num[8];
#ifdef HAVE_REVERSE
	int reverse = 0;
#endif
	int under = 0, bold = 0, color = 0,
		comma, k, i = 0, j = 0, len = strlen (text);
	unsigned char *newtext = malloc (len + 1024);
	struct fe_pg_gui *gui=(struct fe_pg_gui *)sess->gui;
	pghandle pgstr;

	newtext[0] = 0;
	if(text[i]!=3)
	{
		strcpy(newtext, "<font>");
		j=6;
		color=1;
	}
	if (prefs.timestamp)
	{
		j += timecat (newtext+j);
	}
	while (i < len)
	{
		if (dotime && text[i] != 0)
		{
			dotime = FALSE;
			newtext[j] = 0;
			j += timecat (newtext);
		}
		switch (text[i])
		{
		case '<':
			strcpy(&newtext[j], "&lt;");
			j += strlen(newtext+j);
			break;
		case '&':
			strcpy(&newtext[j], "&amp;");
			j += strlen(newtext+j);
			break;
		case 3:
			i++;
			if(color)
			{
				color = FALSE;
				strcpy(&newtext[j], "</font>");
				j += strlen(newtext+j);
			}
			if (!isdigit (text[i]) && color)
				continue;
			k = 0;
			comma = FALSE;
			while (i < len)
			{
				if (text[i] >= '0' && text[i] <= '9' && k < 2)
				{
					num[k] = text[i];
					k++;
				} else
				{
					int mirc;
					num[k] = 0;
					if (k == 0)
					{
						color=TRUE;
						strcpy(&newtext[j], "<font>");
						j += strlen(newtext+j);
					} else
					{
						/* background color not
						 * implemented */
						if (comma)
							break;
						mirc = atoi (num);
						mirc = colconv_rgb[mirc];
						sprintf ((char *) &newtext[j], "<font color=\"#%06x\">", mirc);
						color = TRUE;
						j += strlen (newtext+j);
					}
					if(text[i]==',' && !comma)
						comma = TRUE;
					else
						goto jump;
					k = 0;
				}
				i++;
			}
			break;
		case '\026':				  /* REVERSE */
#ifdef HAVE_REVERSE
			if (reverse)
			{
				reverse = FALSE;
				strcpy (&newtext[j], "\\033[27m");
			} else
			{
				reverse = TRUE;
				strcpy (&newtext[j], "\\033[7m");
			}
			j += strlen (newtext+j);
#endif
			break;
		case '\037':				  /* underline */
			if (under)
			{
				under = FALSE;
				strcpy (&newtext[j], "</ul>");
			} else
			{
				under = TRUE;
				strcpy (&newtext[j], "<ul>");
			}
			j += strlen (newtext+j);
			break;
		case '\002':				  /* bold */
			if (bold)
			{
				bold = FALSE;
				strcpy (&newtext[j], "</b>");
			} else
			{
				bold = TRUE;
				strcpy (&newtext[j], "<b>");
			}
			j += strlen (newtext+j);
			break;
		case '\007':
			if (!prefs.filterbeep)
			{
				fe_beep();
				j++;
			}
			break;
		case '\017':				  /* reset all */
			if (color)
			{
				strcpy (&newtext[j], "</font>");
				color = FALSE;
			}
			else
				newtext[j]=0;
#ifdef HAVE_REVERSE
			if (reverse)
			{
				strcat (&newtext[j], "[unreverse]");
				reverse = FALSE;
			}
#endif
			if (bold)
			{
				strcat (&newtext[j], "</b>");
				bold = FALSE;
			}
			if (under)
			{
				strcat (&newtext[j], "</ul>");
				under = FALSE;
			}
			j += strlen(newtext+j);
			break;
		case '\t':
			newtext[j++] = ' ';
			break;
		case '\n':
			if (prefs.timestamp)
				dotime = TRUE;
		case '\r':
			strcpy(&newtext[j], "<br>");
			j += 4;
			break;
		default:
			newtext[j++] = text[i];
		}
		i++;
jump:
	}
	if (color)
	{
		strcpy (&newtext[j], "</font>");
		color = FALSE;
	}
	else
		newtext[j]=0;
#ifdef HAVE_REVERSE
	if (reverse)
	{
		strcat (&newtext[j], "[unreverse]");
		reverse = FALSE;
	}
#endif
	if (bold)
	{
		strcat (&newtext[j], "</b>");
		bold = FALSE;
	}
	if (under)
	{
		strcat (&newtext[j], "</ul>");
		under = FALSE;
	}

	/* ouput into textbox */
	pgstr=pgNewString(newtext);
	pgSetWidget(gui->output, PG_WP_TEXT, pgstr, 0);
	pgDelete(pgstr);
	free (newtext);
}

/*                            0  1  2  3  4  5  6  7   8   9   10 11  12  13  14 15 */
static int colconv_ansi[] = { 0, 7, 4, 2, 1, 3, 5, 11, 13, 12, 6, 16, 14, 15, 10, 7 };

static void
fe_print_text_terminal (struct session *sess, char *text)
{
	int dotime = FALSE;
	char num[8];
	int reverse = 0, under = 0, bold = 0, color = 0,
		comma, k, i = 0, j = 0, len = strlen (text);
	unsigned char *newtext = malloc (len + 1024);
	struct fe_pg_gui *gui=(struct fe_pg_gui *)sess->gui;

	if (prefs.timestamp)
	{
		newtext[0] = 0;
		j += timecat (newtext);
	}
	while (i < len)
	{
		if (dotime && text[i] != 0)
		{
			dotime = FALSE;
			newtext[j] = 0;
			j += timecat (newtext);
		}
		switch (text[i])
		{
		case 3:
			i++;
			if (!isdigit (text[i]))
			{
				color = FALSE;
				newtext[j] = '\033';
				j++;
				newtext[j] = '[';
				j++;
				newtext[j] = 'm';
				j++;
				continue;
			}
			k = 0;
			comma = FALSE;
			while (i < len)
			{
				if (text[i] >= '0' && text[i] <= '9' && k < 2)
				{
					num[k] = text[i];
					k++;
				} else
				{
					int col, mirc;
					color = TRUE;
					num[k] = 0;
					newtext[j] = '\033';
					j++;
					newtext[j] = '[';
					j++;
					if (k == 0)
					{
						newtext[j] = 'm';
						j++;
					} else
					{
						if (comma)
							col = 40;
						else
							col = 30;
						mirc = atoi (num);
						mirc = colconv_ansi[mirc];
						if (mirc > 9)
						{
							mirc += 50;
							sprintf ((char *) &newtext[j], "%dm", mirc + col);
						} else
						{
							sprintf ((char *) &newtext[j], "%dm", mirc + col);
						}
						j = strlen (newtext);
					}
					if(text[i]==',' && !comma)
						comma = TRUE;
					else
						goto jump;
					k = 0;
				}
				i++;
			}
			break;
		case '\026':				  /* REVERSE */
			if (reverse)
			{
				reverse = FALSE;
				strcpy (&newtext[j], "\033[27m");
			} else
			{
				reverse = TRUE;
				strcpy (&newtext[j], "\033[7m");
			}
			j = strlen (newtext);
			break;
		case '\037':				  /* underline */
			if (under)
			{
				under = FALSE;
				strcpy (&newtext[j], "\033[24m");
			} else
			{
				under = TRUE;
				strcpy (&newtext[j], "\033[4m");
			}
			j = strlen (newtext);
			break;
		case '\002':				  /* bold */
			if (bold)
			{
				bold = FALSE;
				strcpy (&newtext[j], "\033[22m");
			} else
			{
				bold = TRUE;
				strcpy (&newtext[j], "\033[1m");
			}
			j = strlen (newtext);
			break;
		case '\007':
			if (!prefs.filterbeep)
			{
				newtext[j] = text[i];
				j++;
			}
			break;
		case '\017':				  /* reset all */
			strcpy (&newtext[j], "\033[m");
			j += 3;
			reverse = FALSE;
			bold = FALSE;
			under = FALSE;
			break;
		case '\t':
			newtext[j] = ' ';
			j++;
			break;
		case '\n':
			newtext[j] = '\r';
			j++;
			if (prefs.timestamp)
				dotime = TRUE;
		default:
			newtext[j++] = text[i];
		}
		i++;
	 jump:
	}
	newtext[j] = 0;
	if(reverse||bold||under||color)
	{
		strcpy(newtext+j, "\033[m");
		j+=3;
	}

	pgWriteData(gui->output, pgFromTempMemory(newtext,j));
}

void
fe_print_text (struct session *sess, char *text)
{
	struct fe_pg_gui *gui=(struct fe_pg_gui *)sess->gui;

	switch(gui->output_type)
	{
		case PG_WIDGET_TEXTBOX:
			fe_print_text_textbox(sess, text);
			break;
		case PG_WIDGET_TERMINAL:
			fe_print_text_terminal(sess, text);
			break;
		default:
			pgMessageDialog("X-Chat Error",
					"Output of unknown type!", 0);
			break;
	}
}

