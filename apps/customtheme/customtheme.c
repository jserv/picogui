/*
 * Demonstration of fabricating a customized theme to change the appearance
 * of one or more widgets. This example displays a tiled bitmap in its
 * application panel.
 *
 * -- Micah Dowty
 */

#include <picogui.h>
#include <picogui/theme.h>
#include <netinet/in.h>     /* For htonl() and htons() */

/* Theme object for our customization. This needs to be unique
 * arbitrary number that is large enough it will not interfere with
 * builtin theme objects.
 *
 * FIXME: Add an API to retrieve a unique theme object number
 */
#define MY_OBJECT   6502

/******************************** Theme definition */

/* Define the background of a theme object with a tiled bitmap.
 *
 * object: theme object to define
 * bitmap: handle of a bitmap to tile  
 * 
 * This loads the theme into the PicoGUI server and returns a handle to it.
 */
pghandle tiled_bitmap_theme(int object, pghandle bitmap) {
  /* In-memory theme file */
  static struct {
    struct pgtheme_header hdr;          /* Theme header */
    struct pgtheme_thobj obj;           /* Define object */
    struct pgtheme_prop prop;           /* Define object::bgfill */
    struct pgrequest req;               /* Request to load the fillstyle */
    unsigned char fs[10];               /* Define the fillstyle bytecode */
  } theme = {
    /* Fill in what we can here */
    hdr: {
      magic: {'P','G','t','h'},
    },      
    fs: {
      /* The format of these opcodes are documented in constants.h */
      0x10,0x11,0x12,0x13, /* Put x,y,w,h on the stack */
      0x20,0,0,0,0,        /* Put bitmap on the stack- we fill this in later */
      0x94                 /* Bitmap gropnode */
    }
  };

  /* Fill in other parameters */
  theme.hdr.file_len    = htonl(sizeof(theme));
  theme.hdr.file_sum32  = htonl(0);
  theme.hdr.file_ver    = htons(PGTH_FORMATVERSION);
  theme.hdr.num_tags    = htons(0);
  theme.hdr.num_thobj   = htons(1);
  theme.hdr.num_totprop = htons(1);
  theme.obj.id          = htons(object);
  theme.obj.num_prop    = htons(1);
  theme.obj.proplist    = htonl(sizeof(theme.hdr) + sizeof(theme.obj));
  theme.prop.id         = htons(PGTH_P_BGFILL);
  theme.prop.loader     = htons(PGTH_LOAD_REQUEST);
  theme.prop.data       = htonl(sizeof(theme.hdr) + sizeof(theme.obj) +
				sizeof(theme.prop));
  theme.req.type        = htons(PGREQ_MKFILLSTYLE);
  theme.req.id          = htons(0);
  theme.req.size        = htonl(sizeof(theme.fs));
  bitmap                = htonl(bitmap);
  theme.fs[5]           = ((unsigned char*)&bitmap)[0];
  theme.fs[6]           = ((unsigned char*)&bitmap)[1];
  theme.fs[7]           = ((unsigned char*)&bitmap)[2];
  theme.fs[8]           = ((unsigned char*)&bitmap)[3];

  /* Checksum the theme */
  {
    unsigned long sum, len;
    unsigned char *p;
    len = sizeof(theme);
    sum = 0;
    p   = (unsigned char *) &theme;
    for (;len;len--,p++)
      sum += *p;
    theme.hdr.file_sum32 = htonl(sum);
  } 

  /* Load the theme */
  return pgLoadTheme(pgFromMemory(&theme,sizeof(theme)));
}

/******************************** Main program */

int main(int argc, char **argv) {
  pghandle wTB;
  pgInit(argc,argv);

  /* Create a custom theme */
  tiled_bitmap_theme(MY_OBJECT,pgNewBitmap(pgFromFile("data/fogstreaks.pnm")));

  /* Theme our app panel with it */
  pgRegisterApp(PG_APP_NORMAL,"Theme Customization Demo",0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_THOBJ,MY_OBJECT,
	      0);

  /* Some fun stuff */
   
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Hello. This program fabricated a\n"
				     "PicoGUI theme to change the app's background.\n"
				     "\n"
				     "You can do all sorts of fun things with\n"
				     "PicoGUI's powerful 'fillstyle' system."),
	      0);

  wTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);

  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      PG_WP_BITMAP,pgNewBitmap(pgFromFile("data/dustpuppy.pnm")),
	      PG_WP_BITMASK,pgNewBitmap(pgFromFile("data/dustpuppy_mask.pnm")),
	      0);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Nifty!"),
	      0);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("SuperNifty!"),
	      0);

  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Wacky..."),
	      PG_WP_SIDE,PG_S_ALL,
	      0);

  pgEventLoop();
  return 0;
}
