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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef WIN32
#include <sys/timeb.h>
#include <winsock.h>
#include <process.h>
#else
#include <sys/time.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/utsname.h>
#endif
#include <errno.h>
#include "xchat.h"
#include <ctype.h>
#include "util.h"
#include "../../config.h"
#ifdef USING_FREEBSD
#include <sys/sysctl.h>
#endif
#ifdef SOCKS
#include <socks.h>
#endif

#ifndef HAVE_SNPRINTF
#define snprintf g_snprintf
#endif

#ifdef USE_DEBUG

#undef free
#undef malloc
#undef realloc
#undef strdup

int current_mem_usage;

struct mem_block
{
	char *file;
	void *buf;
	int size;
	int line;
	int total;
	struct mem_block *next;
};

struct mem_block *mroot = NULL;

void *
xchat_malloc (int size, char *file, int line)
{
	void *ret;
	struct mem_block *new;

	current_mem_usage += size;
	ret = malloc (size);
	if (!ret)
	{
		printf ("Out of memory! (%d)\n", current_mem_usage);
		exit (255);
	}

	new = malloc (sizeof (struct mem_block));
	new->buf = ret;
	new->size = size;
	new->next = mroot;
	new->line = line;
	new->file = strdup (file);
	mroot = new;

	printf ("%s:%d Malloc'ed %d bytes, now \033[35m%d\033[m\n", file, line,
				size, current_mem_usage);

	return ret;
}

void *
xchat_realloc (char *old, int len, char *file, int line)
{
	char *ret;

	ret = xchat_malloc (len, file, line);
	if (ret)
	{
		strcpy (ret, old);
		xchat_free (old, file, line);
	}
	return ret;
}

void *
xchat_strdup (char *str, char *file, int line)
{
	void *ret;
	struct mem_block *new;
	int size;

	size = strlen (str) + 1;
	current_mem_usage += size;
	ret = malloc (size);
	if (!ret)
	{
		printf ("Out of memory! (%d)\n", current_mem_usage);
		exit (255);
	}
	strcpy (ret, str);

	new = malloc (sizeof (struct mem_block));
	new->buf = ret;
	new->size = size;
	new->next = mroot;
	new->line = line;
	new->file = strdup (file);
	mroot = new;

	printf ("%s:%d strdup (\"%-.40s\") size: %d, total: \033[35m%d\033[m\n",
				file, line, str, size, current_mem_usage);

	return ret;
}

void
xchat_mem_list (void)
{
	struct mem_block *cur, *p;
	GSList *totals = 0;
	GSList *list;

	cur = mroot;
	while (cur)
	{
		list = totals;
		while (list)
		{
			p = list->data;
			if (p->line == cur->line &&
					strcmp (p->file, cur->file) == 0)
			{
				p->total += p->size;
				break;
			}
			list = list->next;
		}
		if (!list)
		{
			cur->total = cur->size;
			totals = g_slist_prepend (totals, cur);
		}
		cur = cur->next;
	}

	fprintf (stderr, "file              line   size    num  total\n");  
	list = totals;
	while (list)
	{
		cur = list->data;
		fprintf (stderr, "%-15.15s %6d %6d %6d %6d\n", cur->file, cur->line,
					cur->size, cur->total/cur->size, cur->total);
		list = list->next;
	}
}

void
xchat_free (void *buf, char *file, int line)
{
	struct mem_block *cur, *last;

	last = NULL;
	cur = mroot;
	while (cur)
	{
		if (buf == cur->buf)
			break;
		last = cur;
		cur = cur->next;
	}
	if (cur == NULL)
	{
		printf ("%s:%d \033[31mTried to free unknown block %lx!\033[m\n",
				  file, line, (unsigned long) buf);
		/*      abort(); */
		free (buf);
		return;
	}
	current_mem_usage -= cur->size;
	printf ("%s:%d Free'ed %d bytes, usage now \033[35m%d\033[m\n",
				file, line, cur->size, current_mem_usage);
	if (last)
		last->next = cur->next;
	else
		mroot = cur->next;
	free (cur->file);
	free (cur);
}

#define malloc(n) xchat_malloc(n, __FILE__, __LINE__)
#define realloc(n, m) xchat_realloc(n, m, __FILE__, __LINE__)
#define free(n) xchat_free(n, __FILE__, __LINE__)
#define strdup(n) xchat_strdup(n, __FILE__, __LINE__)

#endif /* MEMORY_DEBUG */

char *
file_part (char *file)
{
	char *filepart = file;
	if (!file)
		return "";
	while (1)
	{
		switch (*file)
		{
		case 0:
			return (filepart);
		case '/':
#ifdef WIN32
		case '\\':
#endif
			filepart = file + 1;
			break;
		}
		file++;
	}
}

void
path_part (char *file, char *path)
{
	char *filepart = file_part (file);
	*filepart = 0;
	strcpy (path, file);
}

char *				/* like strstr(), but nocase */
nocasestrstr (char *s, char *wanted)
{
	register const size_t len = strlen (wanted);

	if (len == 0)
		return (char *)s;
	while (tolower(*s) != tolower(*wanted) || strncasecmp (s, wanted, len))
		if (*s++ == '\0')
			return (char *)NULL;
	return (char *)s;
}

char *
errorstring (int err)
{
	static char tbuf[16];
	switch (err)
	{
	case -1:
		return "";
	case 0:
		return _("Remote host closed socket");
#ifndef WIN32
	case ECONNREFUSED:
		return _("Connection refused");
	case ENETUNREACH:
	case EHOSTUNREACH:
		return _("No route to host");
	case ETIMEDOUT:
		return _("Connection timed out");
	case EADDRNOTAVAIL:
		return _("Cannot assign that address");
	case ECONNRESET:
#else
	case WSAECONNREFUSED:
		return _("Connection refused");
	case WSAENETUNREACH:
	case WSAEHOSTUNREACH:
		return _("No route to host");
	case WSAETIMEDOUT:
		return _("Connection timed out");
	case WSAEADDRNOTAVAIL:
		return _("Cannot assign that address");
	case WSAECONNRESET:
#endif
		return _("Connection reset by peer");
	}
	sprintf (tbuf, "%d", err);
	return tbuf;
}

int
waitline (int sok, char *buf, int bufsize)
{
	int i = 0;

	while (1)
	{
		if (read (sok, &buf[i], 1) < 1)
			return -1;
		if (buf[i] == '\n' || bufsize == i + 1)
		{
			buf[i] = 0;
			return i;
		}
		i++;
	}
}

/* checks for "~" in a file and expands */

char *
expand_homedir (char *file)
{
#ifndef WIN32
	char *ret;

	if (*file == '~')
	{
		ret = malloc (strlen (file) + strlen (g_get_home_dir ()) + 1);
		sprintf (ret, "%s%s", g_get_home_dir (), file + 1);
		return ret;
	}
#endif
	return strdup (file);
}

unsigned char *
strip_color (unsigned char *text)
{
	int nc = 0;
	int i = 0;
	int col = 0;
	int len = strlen (text);
	unsigned char *new_str = malloc (len + 2);

	while (len > 0)
	{
		if ((col && isdigit (*text) && nc < 2) ||
			 (col && *text == ',' && isdigit (*(text+1)) && nc < 3))
		{
			nc++;
			if (*text == ',')
				nc = 0;
		} else
		{
			col = 0;
			switch (*text)
			{
			case '\003':			  /*ATTR_COLOR: */
				col = 1;
				nc = 0;
				break;
			case '\007':			  /*ATTR_BEEP: */
			case '\017':			  /*ATTR_RESET: */
			case '\026':			  /*ATTR_REVERSE: */
			case '\002':			  /*ATTR_BOLD: */
			case '\037':			  /*ATTR_UNDERLINE: */
				break;
			default:
				new_str[i] = *text;
				i++;
			}
		}
		text++;
		len--;
	}

	new_str[i] = 0;

	return new_str;
}

#if defined (USING_LINUX) || defined (USING_FREEBSD)

static void
get_cpu_info (int *mhz, int *cpus)
{

#ifdef USING_LINUX

	char buf[256];
	int fh;

	*mhz = 0;
	*cpus = 0;

	fh = open ("/proc/cpuinfo", O_RDONLY);	/* linux 2.2+ only */
	if (fh == -1)
	{
		*cpus = 1;
		return;
	}

	while (1)
	{
		if (waitline (fh, buf, sizeof buf) < 0)
			break;
		if (!strncmp (buf, "cycle frequency [Hz]\t:", 22))	/* alpha */
		{
			*mhz = atoi (buf + 23) / 1048576;
		} else if (!strncmp (buf, "cpu MHz\t\t:", 10))	/* i386 */
		{
			*mhz = atof (buf + 11) + 0.5;
		} else if (!strncmp (buf, "clock\t\t:", 8))	/* PPC */
		{
			*mhz = atoi (buf + 9);
		} else if (!strncmp (buf, "processor\t", 10))
		{
			(*cpus)++;
		}
	}
	close (fh);
	if (!*cpus)
		*cpus = 1;

#endif
#ifdef USING_FREEBSD

	int mib[2], ncpu;
	u_long freq;
	size_t len;

	*mhz = 0;
	*cpus = 0;

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;

	len = sizeof(ncpu);
	sysctl(mib, 2, &ncpu, &len, NULL, 0);

	len = sizeof(freq);
	sysctlbyname("machdep.tsc_freq", &freq, &len, NULL, 0);
	
	*cpus = ncpu;
	*mhz = (freq / 1000000);

#endif

}
#endif

#ifdef WIN32

static int
get_mhz (void)
{
	HKEY hKey;
	int result, data, dataSize;

	if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, "Hardware\\Description\\System\\"
		"CentralProcessor\\0", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		dataSize = sizeof (data);
		result = RegQueryValueEx (hKey, "~MHz", 0, 0, (LPBYTE)&data, &dataSize);
		RegCloseKey (hKey);
		if (result == ERROR_SUCCESS)
			return data;
	}
	return 0;	/* fails on Win9x */
}

char *
get_cpu_str (void)
{
	static char verbuf[64];
	OSVERSIONINFO osvi;
	SYSTEM_INFO si;
	int mhz;

	osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	GetVersionEx (&osvi);
	GetSystemInfo (&si);

	mhz = get_mhz ();
	if (mhz)
		sprintf (verbuf, "Windoze %ld.%ld [i%1d86/%dMHz]",
			osvi.dwMajorVersion, osvi.dwMinorVersion, si.wProcessorLevel, mhz);
	else
		sprintf (verbuf, "Windoze %ld.%ld [i%1d86]",
			osvi.dwMajorVersion, osvi.dwMinorVersion, si.wProcessorLevel);

	return verbuf;
}

#else

char *
get_cpu_str (void)
{
#if defined (USING_LINUX) || defined (USING_FREEBSD)
	int mhz, cpus;
#endif
	struct utsname un;
	static char *buf = NULL;

	if (buf)
		return buf;

	buf = malloc (128);

	uname (&un);

#if defined (USING_LINUX) || defined (USING_FREEBSD)
	get_cpu_info (&mhz, &cpus);
	if (mhz)
		snprintf (buf, 128,
					 (cpus == 1) ? "%s %s [%s/%dMHz]" : "%s %s [%s/%dMHz/SMP]",
					 un.sysname, un.release, un.machine, mhz);
	else
#endif
		snprintf (buf, 128, "%s %s [%s]", un.sysname, un.release, un.machine);

	return buf;
}

#endif

int
buf_get_line (char *ibuf, char **buf, int *position, int len)
{
	int pos = *position, spos = pos;

	if (pos == len)
		return 0;

	while (ibuf[pos++] != '\n')
	{
		if (pos == len)
			return 0;
	}
	pos--;
	ibuf[pos] = 0;
	*buf = &ibuf[spos];
	pos++;
	*position = pos;
	return 1;
}

int match(const char *mask, const char *string)
{
  register const char *m = mask, *s = string;
  register char ch;
  const char *bm, *bs;		/* Will be reg anyway on a decent CPU/compiler */

  /* Process the "head" of the mask, if any */
  while ((ch = *m++) && (ch != '*'))
    switch (ch)
    {
      case '\\':
	if (*m == '?' || *m == '*')
	  ch = *m++;
      default:
	if (tolower(*s) != tolower(ch))
	  return 0;
      case '?':
	if (!*s++)
	  return 0;
    };
  if (!ch)
    return !(*s);

  /* We got a star: quickly find if/where we match the next char */
got_star:
  bm = m;			/* Next try rollback here */
  while ((ch = *m++))
    switch (ch)
    {
      case '?':
	if (!*s++)
	  return 0;
      case '*':
	bm = m;
	continue;		/* while */
      case '\\':
	if (*m == '?' || *m == '*')
	  ch = *m++;
      default:
	goto break_while;	/* C is structured ? */
    };
break_while:
  if (!ch)
    return 1;			/* mask ends with '*', we got it */
  ch = tolower(ch);
  while (tolower(*s++) != ch)
    if (!*s)
      return 0;
  bs = s;			/* Next try start from here */

  /* Check the rest of the "chunk" */
  while ((ch = *m++))
  {
    switch (ch)
    {
      case '*':
	goto got_star;
      case '\\':
	if (*m == '?' || *m == '*')
	  ch = *m++;
      default:
	if (tolower(*s) != tolower(ch))
	{
	  m = bm;
	  s = bs;
	  goto got_star;
	};
      case '?':
	if (!*s++)
	  return 0;
    };
  };
  if (*s)
  {
    m = bm;
    s = bs;
    goto got_star;
  };
  return 1;
}

int
for_files (char *dirname, char *mask, void callback (char *file))
{
	DIR *dir;
	struct dirent *ent;
	char *buf;
	int i = 0;

	dir = opendir (dirname);
	if (dir)
	{
		while ((ent = readdir (dir)))
		{
			if (strcmp (ent->d_name, ".") && strcmp (ent->d_name, ".."))
			{
				if (match (mask, ent->d_name))
				{
					i++;
					buf = malloc (strlen (dirname) + strlen (ent->d_name) + 2);
					sprintf (buf, "%s/%s", dirname, ent->d_name);
					callback (buf);
					free (buf);
				}
			}
		}
		closedir (dir);
	}
	return i;
}

void
tolowerStr (char *str)
{
	while (*str)
	{
		*str = tolower (*str);
		str++;
	}
}

/* thanks BitchX */

char *
country (char *hostname)
{
	typedef struct _domain
	{
		char *code;
		char *country;
	}
	Domain;
	static Domain domain[] = {
		{"AD", N_("Andorra") },
		{"AE", N_("United Arab Emirates") },
		{"AF", N_("Afghanistan") },
		{"AG", N_("Antigua and Barbuda") },
		{"AI", N_("Anguilla") },
		{"AL", N_("Albania") },
		{"AM", N_("Armenia") },
		{"AN", N_("Netherlands Antilles") },
		{"AO", N_("Angola") },
		{"AQ", N_("Antarctica") },
		{"AR", N_("Argentina") },
		{"AS", N_("American Samoa") },
		{"AT", N_("Austria") },
		{"AU", N_("Australia") },
		{"AW", N_("Aruba") },
		{"AZ", N_("Azerbaijan") },
		{"BA", N_("Bosnia and Herzegovina") },
		{"BB", N_("Barbados") },
		{"BD", N_("Bangladesh") },
		{"BE", N_("Belgium") },
		{"BF", N_("Burkina Faso") },
		{"BG", N_("Bulgaria") },
		{"BH", N_("Bahrain") },
		{"BI", N_("Burundi") },
		{"BJ", N_("Benin") },
		{"BM", N_("Bermuda") },
		{"BN", N_("Brunei Darussalam") },
		{"BO", N_("Bolivia") },
		{"BR", N_("Brazil") },
		{"BS", N_("Bahamas") },
		{"BT", N_("Bhutan") },
		{"BV", N_("Bouvet Island") },
		{"BW", N_("Botswana") },
		{"BY", N_("Belarus") },
		{"BZ", N_("Belize") },
		{"CA", N_("Canada") },
		{"CC", N_("Cocos Islands") },
		{"CF", N_("Central African Republic") },
		{"CG", N_("Congo") },
		{"CH", N_("Switzerland") },
		{"CI", N_("Cote D'ivoire") },
		{"CK", N_("Cook Islands") },
		{"CL", N_("Chile") },
		{"CM", N_("Cameroon") },
		{"CN", N_("China") },
		{"CO", N_("Colombia") },
		{"CR", N_("Costa Rica") },
		{"CS", N_("Former Czechoslovakia") },
		{"CU", N_("Cuba") },
		{"CV", N_("Cape Verde") },
		{"CX", N_("Christmas Island") },
		{"CY", N_("Cyprus") },
		{"CZ", N_("Czech Republic") },
		{"DE", N_("Germany") },
		{"DJ", N_("Djibouti") },
		{"DK", N_("Denmark") },
		{"DM", N_("Dominica") },
		{"DO", N_("Dominican Republic") },
		{"DZ", N_("Algeria") },
		{"EC", N_("Ecuador") },
		{"EE", N_("Estonia") },
		{"EG", N_("Egypt") },
		{"EH", N_("Western Sahara") },
		{"ER", N_("Eritrea") },
		{"ES", N_("Spain") },
		{"ET", N_("Ethiopia") },
		{"FI", N_("Finland") },
		{"FJ", N_("Fiji") },
		{"FK", N_("Falkland Islands") },
		{"FM", N_("Micronesia") },
		{"FO", N_("Faroe Islands") },
		{"FR", N_("France") },
		{"FX", N_("France, Metropolitan") },
		{"GA", N_("Gabon") },
		{"GB", N_("Great Britain") },
		{"GD", N_("Grenada") },
		{"GE", N_("Georgia") },
		{"GF", N_("French Guiana") },
		{"GG", N_("British Channel Isles") },
		{"GH", N_("Ghana") },
		{"GI", N_("Gibraltar") },
		{"GL", N_("Greenland") },
		{"GM", N_("Gambia") },
		{"GN", N_("Guinea") },
		{"GP", N_("Guadeloupe") },
		{"GQ", N_("Equatorial Guinea") },
		{"GR", N_("Greece") },
		{"GS", N_("S. Georgia and S. Sandwich Isles.") },
		{"GT", N_("Guatemala") },
		{"GU", N_("Guam") },
		{"GW", N_("Guinea-Bissau") },
		{"GY", N_("Guyana") },
		{"HK", N_("Hong Kong") },
		{"HM", N_("Heard and McDonald Islands") },
		{"HN", N_("Honduras") },
		{"HR", N_("Croatia") },
		{"HT", N_("Haiti") },
		{"HU", N_("Hungary") },
		{"ID", N_("Indonesia") },
		{"IE", N_("Ireland") },
		{"IL", N_("Israel") },
		{"IN", N_("India") },
		{"IO", N_("British Indian Ocean Territory") },
		{"IQ", N_("Iraq") },
		{"IR", N_("Iran") },
		{"IS", N_("Iceland") },
		{"IT", N_("Italy") },
		{"JM", N_("Jamaica") },
		{"JO", N_("Jordan") },
		{"JP", N_("Japan") },
		{"KE", N_("Kenya") },
		{"KG", N_("Kyrgyzstan") },
		{"KH", N_("Cambodia") },
		{"KI", N_("Kiribati") },
		{"KM", N_("Comoros") },
		{"KN", N_("St. Kitts and Nevis") },
		{"KP", N_("North Korea") },
		{"KR", N_("South Korea") },
		{"KW", N_("Kuwait") },
		{"KY", N_("Cayman Islands") },
		{"KZ", N_("Kazakhstan") },
		{"LA", N_("Laos") },
		{"LB", N_("Lebanon") },
		{"LC", N_("Saint Lucia") },
		{"LI", N_("Liechtenstein") },
		{"LK", N_("Sri Lanka") },
		{"LR", N_("Liberia") },
		{"LS", N_("Lesotho") },
		{"LT", N_("Lithuania") },
		{"LU", N_("Luxembourg") },
		{"LV", N_("Latvia") },
		{"LY", N_("Libya") },
		{"MA", N_("Morocco") },
		{"MC", N_("Monaco") },
		{"MD", N_("Moldova") },
		{"MG", N_("Madagascar") },
		{"MH", N_("Marshall Islands") },
		{"MK", N_("Macedonia") },
		{"ML", N_("Mali") },
		{"MM", N_("Myanmar") },
		{"MN", N_("Mongolia") },
		{"MO", N_("Macau") },
		{"MP", N_("Northern Mariana Islands") },
		{"MQ", N_("Martinique") },
		{"MR", N_("Mauritania") },
		{"MS", N_("Montserrat") },
		{"MT", N_("Malta") },
		{"MU", N_("Mauritius") },
		{"MV", N_("Maldives") },
		{"MW", N_("Malawi") },
		{"MX", N_("Mexico") },
		{"MY", N_("Malaysia") },
		{"MZ", N_("Mozambique") },
		{"NA", N_("Namibia") },
		{"NC", N_("New Caledonia") },
		{"NE", N_("Niger") },
		{"NF", N_("Norfolk Island") },
		{"NG", N_("Nigeria") },
		{"NI", N_("Nicaragua") },
		{"NL", N_("Netherlands") },
		{"NO", N_("Norway") },
		{"NP", N_("Nepal") },
		{"NR", N_("Nauru") },
		{"NT", N_("Neutral Zone") },
		{"NU", N_("Niue") },
		{"NZ", N_("New Zealand") },
		{"OM", N_("Oman") },
		{"PA", N_("Panama") },
		{"PE", N_("Peru") },
		{"PF", N_("French Polynesia") },
		{"PG", N_("Papua New Guinea") },
		{"PH", N_("Philippines") },
		{"PK", N_("Pakistan") },
		{"PL", N_("Poland") },
		{"PM", N_("St. Pierre and Miquelon") },
		{"PN", N_("Pitcairn") },
		{"PR", N_("Puerto Rico") },
		{"PT", N_("Portugal") },
		{"PW", N_("Palau") },
		{"PY", N_("Paraguay") },
		{"QA", N_("Qatar") },
		{"RE", N_("Reunion") },
		{"RO", N_("Romania") },
		{"RU", N_("Russian Federation") },
		{"RW", N_("Rwanda") },
		{"SA", N_("Saudi Arabia") },
		{"Sb", N_("Solomon Islands") },
		{"SC", N_("Seychelles") },
		{"SD", N_("Sudan") },
		{"SE", N_("Sweden") },
		{"SG", N_("Singapore") },
		{"SH", N_("St. Helena") },
		{"SI", N_("Slovenia") },
		{"SJ", N_("Svalbard and Jan Mayen Islands") },
		{"SK", N_("Slovak Republic") },
		{"SL", N_("Sierra Leone") },
		{"SM", N_("San Marino") },
		{"SN", N_("Senegal") },
		{"SO", N_("Somalia") },
		{"SR", N_("Suriname") },
		{"ST", N_("Sao Tome and Principe") },
		{"SU", N_("Former USSR") },
		{"SV", N_("El Salvador") },
		{"SY", N_("Syria") },
		{"SZ", N_("Swaziland") },
		{"TC", N_("Turks and Caicos Islands") },
		{"TD", N_("Chad") },
		{"TF", N_("French Southern Territories") },
		{"TG", N_("Togo") },
		{"TH", N_("Thailand") },
		{"TJ", N_("Tajikistan") },
		{"TK", N_("Tokelau") },
		{"TM", N_("Turkmenistan") },
		{"TN", N_("Tunisia") },
		{"TO", N_("Tonga") },
		{"TP", N_("East Timor") },
		{"TR", N_("Turkey") },
		{"TT", N_("Trinidad and Tobago") },
		{"TV", N_("Tuvalu") },
		{"TW", N_("Taiwan") },
		{"TZ", N_("Tanzania") },
		{"UA", N_("Ukraine") },
		{"UG", N_("Uganda") },
		{"UK", N_("United Kingdom") },
		{"UM", N_("US Minor Outlying Islands") },
		{"US", N_("United States of America") },
		{"UY", N_("Uruguay") },
		{"UZ", N_("Uzbekistan") },
		{"VA", N_("Vatican City State") },
		{"VC", N_("St. Vincent and the grenadines") },
		{"VE", N_("Venezuela") },
		{"VG", N_("British Virgin Islands") },
		{"VI", N_("US Virgin Islands") },
		{"VN", N_("Vietnam") },
		{"VU", N_("Vanuatu") },
		{"WF", N_("Wallis and Futuna Islands") },
		{"WS", N_("Samoa") },
		{"YE", N_("Yemen") },
		{"YT", N_("Mayotte") },
		{"YU", N_("Yugoslavia") },
		{"ZA", N_("South Africa") },
		{"ZM", N_("Zambia") },
		{"ZR", N_("Zaire") },
		{"ZW", N_("Zimbabwe") },
		{"COM", N_("Internic Commercial") },
		{"EDU", N_("Educational Institution") },
		{"GOV", N_("Government") },
		{"INT", N_("International") },
		{"MIL", N_("Military") },
		{"NET", N_("Internic Network") },
		{"ORG", N_("Internic Non-Profit Organization") },
		{"RPA", N_("Old School ARPAnet") },
		{"ATO", N_("Nato Fiel") },
		{"MED", N_("United States Medical") },
		{"ARPA", N_("Reverse DNS") },
		{NULL, NULL}
	};
	char *p;
	int i;
	if (!hostname || !*hostname || isdigit (hostname[strlen (hostname) - 1]))
		return _("unknown");
	if ((p = strrchr (hostname, '.')))
		p++;
	else
		p = hostname;
	for (i = 0; domain[i].code; i++)
		if (!strcasecmp (p, domain[i].code))
			return domain[i].country;
	return _("unknown");
}

/* I think gnome1.0.x isn't necessarily linked against popt, ah well! */
/* !!! For now use this inlined function, or it would break fe-text building */
/* .... will find a better solution later. */
/*#ifndef USE_GNOME*/

/* this is taken from gnome-libs 1.2.4 */
#define POPT_ARGV_ARRAY_GROW_DELTA 5

static int my_poptParseArgvString(const char * s, int * argcPtr, char *** argvPtr) {
    char * buf, * bufStart, * dst;
    const char * src;
    char quote = '\0';
    int argvAlloced = POPT_ARGV_ARRAY_GROW_DELTA;
    char ** argv = malloc(sizeof(*argv) * argvAlloced);
    const char ** argv2;
    int argc = 0;
    int i, buflen;

    buflen = strlen(s) + 1;
/*    bufStart = buf = alloca(buflen);*/
	 bufStart = buf = malloc (buflen);
    memset(buf, '\0', buflen);

    src = s;
    argv[argc] = buf;

    while (*src) {
	if (quote == *src) {
	    quote = '\0';
	} else if (quote) {
	    if (*src == '\\') {
		src++;
		if (!*src) {
		    free(argv);
			 free(bufStart);
		    return 1;
		}
		if (*src != quote) *buf++ = '\\';
	    }
	    *buf++ = *src;
	} else if (isspace(*src)) {
	    if (*argv[argc]) {
		buf++, argc++;
		if (argc == argvAlloced) {
		    argvAlloced += POPT_ARGV_ARRAY_GROW_DELTA;
		    argv = realloc(argv, sizeof(*argv) * argvAlloced);
		}
		argv[argc] = buf;
	    }
	} else switch (*src) {
	  case '"':
	  case '\'':
	    quote = *src;
	    break;
	  case '\\':
	    src++;
	    if (!*src) {
		free(argv);
		free(bufStart);
		return 1;
	    }
	    /* fallthrough */
	  default:
	    *buf++ = *src;
	}

	src++;
    }

    if (strlen(argv[argc])) {
	argc++, buf++;
    }

    dst = malloc((argc + 1) * sizeof(*argv) + (buf - bufStart));
    argv2 = (void *) dst;
    dst += (argc + 1) * sizeof(*argv);
    memcpy((void *)argv2, argv, argc * sizeof(*argv));
    argv2[argc] = NULL;
    memcpy(dst, bufStart, buf - bufStart);

    for (i = 0; i < argc; i++) {
	argv2[i] = dst + (argv[i] - bufStart);
    }

    free(argv);

    *argvPtr = (char **)argv2;	/* XXX don't change the API */
    *argcPtr = argc;

	 free (bufStart);

    return 0;
}

int
util_exec (char *cmd)
{
	int pid;
	char **argv;
	int argc;

	if (my_poptParseArgvString (cmd, &argc, &argv) != 0)
		return -1;

#ifndef WIN32
	pid = fork ();
	if (pid == -1)
		return -1;
	if (pid == 0)
	{
		execvp (argv[0], argv);
		_exit (0);
	} else
	{
		free (argv);
		return pid;
	}
#else
	spawnvp (_P_DETACH, argv[0], argv);
	free (argv);
	return 0;
#endif
}

unsigned long
make_ping_time (void)
{
#ifndef WIN32
	struct timeval timev;
	gettimeofday (&timev, 0);
#else
	GTimeVal timev;
	g_get_current_time (&timev);
#endif
	return (timev.tv_sec - 50000) * 1000000 + timev.tv_usec;
}


/************************************************************************
 *    This technique was borrowed in part from the source code to 
 *    ircd-hybrid-5.3 to implement case-insensitive string matches which
 *    are fully compliant with Section 2.2 of RFC 1459, the copyright
 *    of that code being (C) 1990 Jarkko Oikarinen and under the GPL.
 *    
 *    A special thanks goes to Mr. Okarinen for being the one person who
 *    seems to have ever noticed this section in the original RFC and
 *    written code for it.  Shame on all the rest of you (myself included).
 *    
 *        --+ Dagmar d'Surreal
 */

int
rfc_casecmp (char *s1, char *s2)
{
	register unsigned char *str1 = (unsigned char *) s1;
	register unsigned char *str2 = (unsigned char *) s2;
	register int res;

	while ((res = tolower (*str1) - tolower (*str2)) == 0)
	{
		if (*str1 == '\0')
			return 0;
		str1++;
		str2++;
	}
	return (res);
}

int
rfc_ncasecmp (char *str1, char *str2, int n)
{
	register unsigned char *s1 = (unsigned char *) str1;
	register unsigned char *s2 = (unsigned char *) str2;
	register int res;

	while ((res = tolower (*s1) - tolower (*s2)) == 0)
	{
		s1++;
		s2++;
		n--;
		if (n == 0 || (*s1 == '\0' && *s2 == '\0'))
			return 0;
	}
	return (res);
}

unsigned char tolowertab[] =
	{ 0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
	0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
	0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
	0x1e, 0x1f,
	' ', '!', '"', '#', '$', '%', '&', 0x27, '(', ')',
	'*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	':', ';', '<', '=', '>', '?',
	'@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
	'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
	't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
	'_',
	'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
	'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
	't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
	0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
	0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
	0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
	0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
	0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
	0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
	0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

/*static unsigned char touppertab[] =
	{ 0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
	0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
	0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
	0x1e, 0x1f,
	' ', '!', '"', '#', '$', '%', '&', 0x27, '(', ')',
	'*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
	'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
	'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^',
	0x5f,
	'`', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
	'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
	'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^',
	0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
	0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
	0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
	0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
	0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
	0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
	0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};*/
