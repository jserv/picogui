/* cfgfiles.h */

extern char *xdir;

char *cfg_get_str (char *cfg, char *var, char *dest);
int cfg_get_bool (char *var);
int cfg_get_int_with_result (char *cfg, char *var, int *result);
int cfg_get_int (char *cfg, char *var);
void cfg_put_int (int fh, int value, char *var);
char *get_xdir (void);
char *default_file (void);
void check_prefs_dir (void);
void load_config (void);
int save_config (void);
void list_free (GSList ** list);
void list_loadconf (char *file, GSList ** list, char *defaultconf);
int list_delentry (GSList ** list, char *name);
void list_addentry (GSList ** list, char *cmd, char *name);

#define STRUCT_OFFSET_STR(type,field) \
( (unsigned int) (((char *) (&(((type *) NULL)->field)))- ((char *) NULL)) )

#define STRUCT_OFFSET_INT(type,field) \
( (unsigned int) (((int *) (&(((type *) NULL)->field)))- ((int *) NULL)) )

#define PREFS_OFFSET(field) STRUCT_OFFSET_STR(struct xchatprefs, field)
#define PREFS_OFFINT(field) STRUCT_OFFSET_INT(struct xchatprefs, field)

struct prefs
{
	char *name;
	int offset;
	int type;
};

#define TYPE_STR 1
#define TYPE_INT 2
#define TYPE_BOOL 3
