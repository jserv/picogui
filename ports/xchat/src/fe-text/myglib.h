#include <limits.h>
#include <assert.h>

typedef void *gpointer;
typedef int gint;
typedef unsigned int guint;
typedef char gchar;
typedef unsigned long gulong;
typedef unsigned long guint32;
typedef unsigned long long guint64;
typedef float gfloat;
typedef int gboolean;
typedef void GIOChannel;
typedef int GIOCondition;

typedef void (*GFunc) (void *wid, void *data);

struct gslist
{
	gpointer data;
	gpointer next;
};
typedef struct gslist GSList;

#define g_assert assert
#define g_malloc malloc
#define g_free free
#define g_strdup strdup
#define g_new(a,b) malloc(sizeof(a)*b)
#define g_new0(a,b) calloc(b,sizeof(a))

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

#define g_return_if_fail
#define G_LOCK
#define G_UNLOCK

extern int glib_major_version;
extern int glib_minor_version;
extern int glib_micro_version;


char *g_get_user_name ();
char *g_get_home_dir ();

GSList *g_slist_alloc (void);
void g_slist_free (GSList * list);
GSList *g_slist_last (GSList * list);
GSList *g_slist_append (GSList * list, gpointer data);
GSList *g_slist_prepend (GSList * list, gpointer data);
GSList *g_slist_insert (GSList * list, gpointer data, gint position);
GSList *g_slist_remove (GSList * list, gpointer data);
void g_slist_foreach (GSList * list, GFunc func, gpointer user_data);
gint g_snprintf (gchar * str, gulong n, gchar const *fmt, ...);
