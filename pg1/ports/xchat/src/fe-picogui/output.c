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

#include <picogui.h>
#include <picogui/theme.h>

#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/cfgfiles.h"
#include "../common/fe.h"

#include "fe-picogui.h"

int ircterm=PGTH_O_TERMINAL;
static const char termthemename[]="IRCterm";

/* May need to use char* on some compilers */
#define offsetof(structure, member) ((void*)&structure.member-(void*)&structure)

#define TOTAL_COLORS 16

static pgcolor colconv_rgb[TOTAL_COLORS] = { 0xcfcfcf, 0x000000, 0x0000cc,
	0x00cc00, 0xdd0000, 0xaa0000, 0xbb00bb, 0xffaa00, 0xeedd22, 0x33de55,
	0x00cccc, 0x33ddee, 0x0000ff, 0xee22ee, 0x777777, 0x999999 };
static const char *colconv_termfg[TOTAL_COLORS] = { "22;30", "22;34", "22;32",
	"22;36", "22;31", "22;35", "22;33", "22;37", "1;30", "1;34", "1;32",
	"1;36", "1;31", "1;35", "1;33", "1;37" };
static const char *colconv_termbg[TOTAL_COLORS] = { "25;40", "25;44", "25;42",
	"25;46", "25;41", "25;45", "25;43", "25;47", "5;40", "5;44", "5;42",
	"5;46", "5;41", "5;45", "5;43", "5;47" };
/* PicoGUI theme for terminal palette */
struct termtheme_struct {
	struct pgtheme_header hdr;
	struct pgtheme_thobj obj;
	struct pgtheme_prop nameprop, parent, def, palette;
	struct pgrequest namereq;
	char name[sizeof termthemename-1];
	struct pgrequest palreq;
	pgcolor array[TOTAL_COLORS];
};

void palette_load(void)
{
	int i, fh, res, red, green, blue;
	struct stat st;
	char prefname[256], *cfg;
	static pghandle themehandle=0;

	/* PicoGUI theme for terminal palette */
	struct termtheme_struct termtheme;

	i=snprintf(prefname, sizeof prefname, "%s/palette.conf", get_xdir());
	if(i>0&&i<sizeof prefname)
	{
		fh=open(prefname, O_RDONLY|OFLAGS);
		if(fh>=0)
		{
			fstat(fh, &st);
			cfg = malloc(st.st_size+1);
			if(cfg!=NULL)
			{
				i=read(fh, cfg, st.st_size);
				if(i>0) for(i=0; i<TOTAL_COLORS; i++)
				{
					snprintf(prefname, sizeof prefname,
							"color_%d_red", i);
					red=cfg_get_int(cfg, prefname);
					snprintf(prefname, sizeof prefname,
							"color_%d_grn", i);
					green=cfg_get_int(cfg, prefname);
					snprintf(prefname, sizeof prefname,
							"color_%d_blu", i);
					blue=cfg_get_int_with_result(cfg,
							prefname, &res);
					if(res)
					{
						colconv_rgb[i]=
							((red&0xff00)<<8) |
							(green&0xff00) |
							(blue>>8);
					}
				}
				free(cfg);
			}
			close(fh);
		}
	}
	/* build terminal palette theme */
	memset(&termtheme, 0, sizeof termtheme);
	termtheme.hdr.magic[0]='P';
	termtheme.hdr.magic[1]='G';
	termtheme.hdr.magic[2]='t';
	termtheme.hdr.magic[3]='h';
	termtheme.hdr.file_len=htonl(sizeof termtheme);
	termtheme.hdr.file_ver=htons(PGTH_FORMATVERSION);
	termtheme.hdr.num_thobj=htons(1);
	termtheme.hdr.num_totprop=htons(4);
	termtheme.obj.id=htons(PGTH_O_CUSTOM);
	termtheme.obj.num_prop=htons(4);
	termtheme.obj.proplist=htonl(offsetof(termtheme, nameprop));

	termtheme.nameprop.id=htons(PGTH_P_NAME);
	termtheme.nameprop.loader=htons(PGTH_LOAD_REQUEST);
	termtheme.nameprop.data=htonl(offsetof(termtheme, namereq));
	termtheme.namereq.type=htons(PGREQ_MKSTRING);
	termtheme.namereq.size=htonl(strlen(termthemename));
	memcpy(termtheme.name, termthemename, sizeof termtheme.name);

	termtheme.parent.id=htons(PGTH_P_PARENT);
	termtheme.parent.loader=htons(PGTH_LOAD_NONE);
	termtheme.parent.data=htonl(PGTH_O_TERMINAL);

	termtheme.def.id=htons(PGTH_P_ATTR_DEFAULT);
	termtheme.def.loader=htons(PGTH_LOAD_NONE);
	termtheme.def.data=htonl(0x00000001);

	termtheme.palette.id=htons(PGTH_P_TEXTCOLORS);
	termtheme.palette.loader=htons(PGTH_LOAD_REQUEST);
	termtheme.palette.data=htonl(offsetof(termtheme, palreq));
	termtheme.palreq.type=htons(PGREQ_MKARRAY);
	termtheme.palreq.size=htonl(sizeof termtheme.array);
	for(i=0;i<TOTAL_COLORS;i++)
		termtheme.array[i]=htonl(colconv_rgb[i]);

	{	/* checksum the theme */
		u32 sum, len;
		unsigned char *p;
		len=sizeof(termtheme);
		sum=0;
		p=(unsigned char *)&termtheme;
		for (;len;len--,p++)
			sum+=*p;
		termtheme.hdr.file_sum32=htonl(sum);
	}
	if(themehandle)
		pgDelete(themehandle);
#if 0
	{
		FILE *f;
		f=fopen("ircterm.th", "w");
		fwrite(&termtheme, sizeof termtheme, 1, f);
		fclose(f);
	}
#endif
	themehandle=pgLoadTheme(pgFromMemory(&termtheme, sizeof termtheme));
	ircterm=pgFindThemeObject(termthemename);
	if(!ircterm)
		ircterm=PGTH_O_TERMINAL;	/* fallback to ugly */
}

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
				strcpy (&newtext[j], "\\e[27m");
			} else
			{
				reverse = TRUE;
				strcpy (&newtext[j], "\\e[7m");
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
	pgSetWidget(sess->gui->output, PG_WP_TEXT, pgstr, 0);
	pgDelete(pgstr);
	free (newtext);
}

static void
fe_print_text_terminal (struct session *sess, char *text)
{
	int dotime = FALSE;
	char num[8];
	int reverse = 0, under = 0, bold = 0, color = 0,
		comma, k, i = 0, j = 2, len = strlen (text);
	unsigned char *newtext = malloc (len + 1024);

	newtext[0] = '\r';
	newtext[1] = '\n';
	if (prefs.timestamp)
	{
		newtext[2] = 0;
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
				strcpy (&newtext[j], "\e[m");
				j += 3;
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
					int mirc;
					const char *col;
					color = TRUE;
					num[k] = 0;
					newtext[j++] = '\e';
					newtext[j++] = '[';
					if (k == 0)
					{
						if(comma)
							newtext[j++] = '4';
						else
							newtext[j++] = '3';
						newtext[j++] = '9';
						newtext[j++] = 'm';
					} else
					{
						mirc = atoi (num);
						if (comma)
							col = colconv_termbg[mirc];
						else
							col = colconv_termfg[mirc];
						sprintf ((char *) &newtext[j], "%sm", col);
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
			if (reverse)
			{
				reverse = FALSE;
				strcpy (&newtext[j], "\e[27m");
			} else
			{
				reverse = TRUE;
				strcpy (&newtext[j], "\e[7m");
			}
			j += strlen (newtext+j);
			break;
		case '\037':				  /* underline */
			if (under)
			{
				under = FALSE;
				strcpy (&newtext[j], "\e[24m");
			} else
			{
				under = TRUE;
				strcpy (&newtext[j], "\e[4m");
			}
			j += strlen (newtext+j);
			break;
			/* NOTE: ISO 6429 redefines bold to mean bright text.
			 * Therefore we cannot support bold here? */
		case '\002':				  /* bold */
#if 0
			if (bold)
			{
				bold = FALSE;
				strcpy (&newtext[j], "\e[22m");
			} else
			{
				bold = TRUE;
				strcpy (&newtext[j], "\e[1m");
			}
			j += strlen (newtext+j);
#endif
			break;
		case '\007':
			if (!prefs.filterbeep)
			{
				newtext[j++] = text[i];
			}
			break;
		case '\017':				  /* reset all */
			strcpy (&newtext[j], "\e[m");
			j += 3;
			reverse = FALSE;
			bold = FALSE;
			under = FALSE;
			color = FALSE;
			break;
		case '\t':
			newtext[j++] = ' ';
			break;
		case '\n':
			newtext[j++] = '\r';
			if (prefs.timestamp)
				dotime = TRUE;
		default:
			newtext[j++] = text[i];
		}
		i++;
	 jump:
	}
	if(newtext[j-1]=='\n')
		j-=2;
	newtext[j] = 0;
	if(reverse||bold||under||color)
	{
		strcpy(newtext+j, "\e[m");
		j+=3;
	}

#if 0
	{
		FILE *f;
		f=fopen("outputlog", "a");
		if(f)
		{
			fwrite(newtext, j, 1, f);
			fclose(f);
		}
	}
#endif

	pgWriteData(sess->gui->output, pgFromTempMemory(newtext,j));
}

void
fe_print_text (struct session *sess, char *text)
{
	switch(sess->gui->output_type)
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

