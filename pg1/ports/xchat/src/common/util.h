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

#undef tolower
#define tolower(c) (tolowertab[(unsigned char)(c)])

#define strcasecmp      rfc_casecmp
#define strncasecmp     rfc_ncasecmp

extern unsigned char tolowertab[];

char *expand_homedir (char *file);
void path_part (char *file, char *path);
int match (const char *mask, const char *string);
char *file_part (char *file);
int for_files (char *dirname, char *mask, void callback (char *file));
int rfc_casecmp (char *, char *);
int rfc_ncasecmp (char *, char *, int);
void toupperStr (char *str);
void tolowerStr (char *str);
int buf_get_line (char *, char **, int *, int len);
char *nocasestrstr (char *text, char *tofind);
char *country (char *);
char *get_cpu_str (void);
int util_exec (char *cmd);
unsigned char *strip_color (unsigned char *text);
char *errorstring (int err);
int waitline (int sok, char *buf, int bufsize);
unsigned long make_ping_time (void);
