/*
 * Simple image viewer, using a smidgen of custom themeing
 *
 * -- Micah Dowty <micahjd@users.sourceforge.net>
 */

#include <picogui.h>
#include <string.h>
#include <imgview.th.h>

pghandle panel, bitmapwidget;

void load_bitmap(const char *filename) {
  int w,h;
  pghandle bitmap;
  const char *name;

  /* Delete old bitmap */
  pgDelete(pgGetWidget(bitmapwidget, PG_WP_BITMAP));

  /* Find the actual name of the file */
  name = strrchr(filename,'/');
  if (name)
    name++;
  else
    name = filename;
  
  bitmap = pgNewBitmap(pgFromFile(filename));
  pgSizeBitmap(&w,&h,bitmap);
  pgReplaceTextFmt(panel,"%s - %dx%d Image",name,w,h);
  pgSetWidget(bitmapwidget,PG_WP_BITMAP, bitmap,0);
}

int btnOpen(struct pgEvent *evt) {
  const char *s;
  
  s = pgFilePicker(NULL,NULL,NULL,PG_FILEOPEN,"Open an Image");
  if (s)
    load_bitmap(s);
  return 0;
}

int main(int argc, char **argv) {
  pghandle titleLabel,box,toolbar;
  
  pgInit(argc,argv);
  pgLoadTheme(pgFromMemory(imgviewtheme_bits,imgviewtheme_len));

  panel = pgRegisterApp(PG_APP_NORMAL,"Image Viewer",0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_THOBJ, pgFindThemeObject("imgview.panel"),
	      0);

  box = pgNewWidget(PG_WIDGET_SCROLLBOX, 0, 0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_THOBJ, pgFindThemeObject("imgview.box"),
	      0);

  bitmapwidget = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,box);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_ALL,
	      PG_WP_MARGIN, 0,
	      0);

  titleLabel = pgGetWidget(panel, PG_WP_PANELBAR_LABEL);
  if (!titleLabel) {
    /* If we don't have a panelbar, make a toolbar */
    toolbar = pgNewWidget(PG_WIDGET_TOOLBAR,PG_DERIVE_INSIDE,panel);
  }

  pgNewWidget(PG_WIDGET_BUTTON,
	      titleLabel ? PG_DERIVE_BEFORE : PG_DERIVE_INSIDE,
	      titleLabel ? titleLabel : toolbar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_LEFT,
	      PG_WP_TEXT, pgNewString("Open"),
	      0);
  pgBind(PGDEFAULT, PG_WE_ACTIVATE, btnOpen, &bitmapwidget);

  if (argc > 1)
    load_bitmap(argv[1]);
 
  pgEventLoop();
  return 0;
}

/* The End */
