extern int ircterm;

void palette_load(void);

struct uhmapping {
	struct User *user;
	pghandle handle;
};

struct userbut {
	const char *cmd;
	pghandle h;
};

struct session_gui {
	pghandle app, topic, output, input, userscroll,
		userlistinfo, userlist, buttonbox;
	u16 output_type;
	struct userbut *userbutton;
	struct uhmapping *uhmap;
	int users, buttons;
};

struct server_gui {
	/* not yet implemented */
	pghandle app;
};

typedef int (*socket_callback) (void *source, int condition, void *user_data);
typedef int (*timer_callback) (void *user_data);

struct socketeventRec
{
	socket_callback callback;
	void *userdata;
	int sok;
	int tag;
	int rread:1;
	int wwrite:1;
	int eexcept:1;
	int checked:1;
};

typedef struct socketeventRec socketevent;


struct timerRec
{
	timer_callback callback;
	void *userdata;
	int interval;
	int tag;
	guint64 next_call;	/* miliseconds */
};

typedef struct timerRec timerevent;
