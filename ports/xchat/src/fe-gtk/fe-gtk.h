#include "../../config.h"

#ifndef WIN32
#include <sys/types.h>
#include <regex.h>
#endif

#ifdef USE_GNOME

#undef _
#include <gnome.h>
#undef GNOME_APP
#define GNOME_APP(n) ((GnomeApp*)n)

#else

#include <gtk/gtk.h>
#include "fake_gnome.h"
#if defined(ENABLE_NLS) && !defined(_)
#  include <libintl.h>
#  define _(x) gettext(x)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#endif
#if !defined(ENABLE_NLS) && defined(_)
#  undef _
#  define N_(String) (String)
#  define _(x) (x)
#endif

#endif

#undef GTK_BIN
#undef GTK_WINDOW
#undef GTK_BOX
#undef GTK_OBJECT
#undef GTK_CONTAINER
#undef GTK_CLIST
#undef GTK_TEXT
#undef GTK_LABEL
#undef GTK_ENTRY
#undef GTK_WIDGET
#undef GTK_MENU_BAR
#undef GTK_DIALOG
#undef GTK_FILE_SELECTION
#undef GTK_PANED
#undef GTK_TABLE
#undef GTK_DRAWING_AREA
#undef GTK_FONT_SELECTION_DIALOG
#undef GTK_SCROLLED_WINDOW
#undef GTK_TOGGLE_BUTTON
#undef GTK_NOTEBOOK
#undef GTK_MENU_ITEM
#undef GTK_OPTION_MENU
#undef GTK_MENU
#undef GTK_CTREE
#undef GTK_COLOR_SELECTION_DIALOG
#undef GTK_EDITABLE
#undef GTK_RANGE
#undef GTK_CHECK_MENU_ITEM
#undef GTK_MISC
#undef GTK_FRAME
#undef GTK_BUTTON_BOX

#define GTK_BIN(n) ((GtkBin *)n)
#define GTK_WINDOW(n) ((GtkWindow *)n)
#define GTK_BOX(n) ((GtkBox *)n)
#define GTK_OBJECT(n) ((GtkObject *)n)
#define GTK_CONTAINER(n) ((GtkContainer *)n)
#define GTK_CLIST(n) ((GtkCList *)n)
#define GTK_TEXT(n) ((GtkText *)n)
#define GTK_LABEL(n) ((GtkLabel *)n)
#define GTK_ENTRY(n) ((GtkEntry *)n)
#define GTK_WIDGET(n) ((GtkWidget *)n)
#define GTK_MENU_BAR(n) ((GtkMenuBar *)n)
#define GTK_DIALOG(n) ((GtkDialog *)n)
#define GTK_FILE_SELECTION(n) ((GtkFileSelection *)n)
#define GTK_PANED(n) ((GtkPaned *)n)
#define GTK_TABLE(n) ((GtkTable *)n)
#define GTK_DRAWING_AREA(n) ((GtkDrawingArea *)n)
#define GTK_FONT_SELECTION_DIALOG(n) ((GtkFontSelectionDialog *)n)
#define GTK_SCROLLED_WINDOW(n) ((GtkScrolledWindow *)n)
#define GTK_TOGGLE_BUTTON(n) ((GtkToggleButton *)n)
#define GTK_NOTEBOOK(n) ((GtkNotebook*)n)
#define GTK_MENU_ITEM(n) ((GtkMenuItem*)n)
#define GTK_OPTION_MENU(n) ((GtkOptionMenu *)n)
#define GTK_MENU(n) ((GtkMenu *)n)
#define GTK_CTREE(n) ((GtkCTree*)n)
#define GTK_COLOR_SELECTION_DIALOG(n) ((GtkColorSelectionDialog *)n)
#define GTK_EDITABLE(n) ((GtkEditable *)n)
#define GTK_RANGE(n) ((GtkRange *)n)
#define GTK_CHECK_MENU_ITEM(n) ((GtkCheckMenuItem *)n)
#define GTK_MISC(n) ((GtkMisc *)n)
#define GTK_FRAME(n) ((GtkFrame *)n)
#define GTK_BUTTON_BOX(n) ((GtkButtonBox *)n)

#define flag_t flag_wid[0]
#define flag_n flag_wid[1]
#define flag_s flag_wid[2]
#define flag_i flag_wid[3]
#define flag_p flag_wid[4]
#define flag_m flag_wid[5]
#define flag_l flag_wid[6]
#define flag_k flag_wid[7]
#define flag_b flag_wid[8]
#define NUM_FLAG_WIDS 9

struct server_gui
{
	GtkWidget *rawlog_window;
	GtkWidget *rawlog_textlist;

	/* chanlist variables */
	GtkWidget *chanlist_wild;
	GtkWidget *chanlist_window;
	GtkWidget *chanlist_list;
	GtkWidget *chanlist_refresh;
	GtkWidget *chanlist_label;

	GSList *chanlist_data_stored_rows;	/* stored list so it can be resorted  */

	gchar chanlist_wild_text[256];	/* text for the match expression */

	gboolean chanlist_match_wants_channel;	/* match in channel name */
	gboolean chanlist_match_wants_topic;	/* match in topic */

#ifndef WIN32
	regex_t chanlist_match_regex;	/* compiled regular expression here */
#else
	char *chanlist_match_regex;
#endif

	guint chanlist_users_found_count;	/* users total for all channels */
	guint chanlist_users_shown_count;	/* users total for displayed channels */
	guint chanlist_channels_found_count;	/* channel total for /LIST operation */
	guint chanlist_channels_shown_count;	/* total number of displayed 
														   channels */
	gint chanlist_last_column;	  /* track the last list column user clicked */

	GtkSortType chanlist_sort_type;

	int chanlist_maxusers;
	int chanlist_minusers;
};

struct session_gui
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *menu;
	GtkWidget *usermenu;
	GtkWidget *awaymenuitem;
	GtkWidget *tbox;
	GtkWidget *changad;
	GtkWidget *topicgad;
	GtkWidget *textgad;
	GtkWidget *namelistgad;
	GtkWidget *nickgad;
	GtkWidget *inputgad;
	GtkWidget *namelistinfo;
	GtkWidget *namelistinfo_o;
	GtkWidget *namelistinfo_v;
	GtkWidget *namelistinfo_t;
	GtkWidget *paned;
	GtkWidget *vscrollbar;
	GtkWidget *op_box;
	GtkWidget *op_xpm;
	GtkWidget *userlistbox;
	GtkWidget *nl_box;
	GtkWidget *button_box;
	GtkWidget *toolbox;
	GtkWidget *laginfo;
	GtkWidget *throttleinfo;
	GtkWidget *lagometer;
	GtkWidget *throttlemeter;
	GtkWidget *bar;
	GtkWidget *leftpane;
	GtkWidget *confbutton;		  /* conference mode button */
	GtkWidget *beepbutton;
	GtkWidget *flag_wid[NUM_FLAG_WIDS];
	GtkWidget *limit_entry;		  /* +l */
	GtkWidget *key_entry;		  /* +k */
#ifdef USE_PANEL
	GtkWidget *panel_button;
#endif

	/* banlist stuff */
	GtkWidget *banlist_window;
	GtkWidget *banlist_clistBan;
	GtkWidget *banlist_butRefresh;
};

GdkFont *my_font_load (char *fontname);

extern GdkFont *font_normal;
extern GdkFont *dialog_font_normal;
extern GdkPixmap *channelwin_pix;
extern GdkPixmap *dialogwin_pix;
