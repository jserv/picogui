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
  pghandle titleLabel,scroll,box;
  
  pgInit(argc,argv);
  pgLoadTheme(pgFromMemory(imgviewtheme_bits,imgviewtheme_len));

  panel = pgRegisterApp(PG_APP_NORMAL,"Image Viewer",0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_THOBJ, pgFindThemeObject("imgview.panel"),
	      0);

  scroll = pgNewWidget (PG_WIDGET_SCROLL, 0, 0);
  box = pgNewWidget (PG_WIDGET_BOX, 0, 0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_THOBJ, pgFindThemeObject("imgview.box"),
	      PG_WP_SIDE, PG_S_ALL,
	      0);
  pgSetWidget (scroll, PG_WP_BIND, box, 0);
  

  bitmapwidget = pgNewWidget(PG_WIDGET_BITMAP,PG_DERIVE_INSIDE,box);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_ALL,
	      0);

  titleLabel = pgGetWidget(panel, PG_WP_PANELBAR_LABEL);

  pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_BEFORE, titleLabel);
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
