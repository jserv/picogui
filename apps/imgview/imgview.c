/*
 * Modified version of imgview.c, for checking bitmap rendering
 * problems. (and with rotation of image) -- Olivier Bornet
 *
 * Original comment:
 *
 * Simple image viewer, using a smidgen of custom themeing
 *
 * -- Micah Dowty <micahjd@users.sourceforge.net>
 */

#include <picogui.h>
#include <string.h>

/* uncomment this if you want the CANVAS version */
/* commented result of the BITMAP version */
/* #define __WITH_CANVAS__ */

pghandle panel, bitmapwidget;

#ifdef __WITH_CANVAS__
pghandle canvaswidget;
#endif

void rotate_bitmap (pghandle bitmap, const int angle)
{
  int w, h;
  int rotated_w, rotated_h;
  int rotated_x, rotated_y;
  int clip_x1, clip_y1;
  int clip_x2, clip_y2;

  /* get the size of the given bitmap */
  pgSizeBitmap (&w, &h, bitmap);

  /* compute the needed x/y/w/h for the rotated bitmap */
  switch (angle) {
  case 90:
    rotated_w = h;
    rotated_h = w;
    rotated_x = 0;
    rotated_y = rotated_h-1;
    break;

  case 180:
    rotated_w = w;
    rotated_h = h;
    rotated_x = rotated_w-1;
    rotated_y = rotated_h-1;
    break;

  case 270:
    rotated_w = h;
    rotated_h = w;
    rotated_x = rotated_w-1;
    rotated_y = 0;
    break;

  case 0:
  default:
    rotated_w = w;
    rotated_h = h;
    rotated_x = 0;
    rotated_y = 0;
    break;
  }

  /* compute the clip of the source bitmap */
  clip_x1 = 0;
  clip_y1 = 0;
  clip_x2 = rotated_w-1;
  clip_y2 = rotated_h-1;

#ifdef __WITH_CANVAS__
  /* set the clip of the original bitmap to get */
  pgWriteCmd (canvaswidget,
	      PGCANVAS_GROP, 5,
	      PG_GROP_SETSRC,
	      0, 0, w-1, h-1);

  /* set the wanted angle */
  pgWriteCmd (canvaswidget,
	      PGCANVAS_GROP, 2,
	      PG_GROP_SETANGLE,
	      angle);

  /* rotate the bitmap and render it */
  pgWriteCmd (canvaswidget,
	      PGCANVAS_GROP, 6,
	      PG_GROP_ROTATEBITMAP,
	      rotated_x, rotated_y, rotated_w, rotated_h,
	      bitmap);
#else
  
  { 
    pghandle dest_bitmap = pgCreateBitmap (rotated_w, rotated_h);

    /* some things drawn in the destination bitmap, to see it's working OK */
    pgRender (dest_bitmap, PG_GROP_SETCOLOR, 0x0000FF);
    pgRender (dest_bitmap, PG_GROP_RECT,     0, 0, 100, 100);
    pgRender (dest_bitmap, PG_GROP_SETCOLOR, 0xFFFF00);
    pgRender (dest_bitmap, PG_GROP_LINE,     0, 0, 100, 100);
    pgRender (dest_bitmap, PG_GROP_LINE,     100, 0, -100, 100);

    /* set the clip of the original bitmap to get */
    pgRender (dest_bitmap,
	      PGCANVAS_GROP, 5,
	      PG_GROP_SETSRC, 0, 0, w-1, h-1);

    /* set the wanted angle */
    pgRender (dest_bitmap,
	      PGCANVAS_GROP, 2,
	      PG_GROP_SETANGLE, angle);

    /* rotate the bitmap and render it */
    pgRender (dest_bitmap,
	      PGCANVAS_GROP, 6,
	      PG_GROP_ROTATEBITMAP, 
	      rotated_x, rotated_y, rotated_w, rotated_h,
	      bitmap);

    /* delete old image */
    pgDelete (pgGetWidget (bitmapwidget, PG_WP_BITMAP));

    /* set the image in the final widget */
    pgSetWidget (bitmapwidget, PG_WP_BITMAP, dest_bitmap, 0);
  }
  
#endif

}
  
void load_bitmap (const char *filename, const int angle) {
  int w,h;
  pghandle bitmap;
  const char *name;

  /* Find the actual name of the file */
  name = strrchr(filename,'/');
  if (name)
    name++;
  else
    name = filename;
  
  bitmap = pgNewBitmap(pgFromFile(filename));

  rotate_bitmap (bitmap, angle);
}

int main(int argc, char **argv) {
  pghandle titleLabel,box,toolbar;

  /* usage check */
  if (argc != 3) {
    fprintf (stderr, "Usage: %s image angle\n", argv [0]);
    return 1;
  }

  pgInit(argc,argv);

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

#ifdef __WITH_CANVAS__
  /* a canvas to write in */
  canvaswidget = pgNewWidget (PG_WIDGET_CANVAS,PG_DERIVE_INSIDE, bitmapwidget);  
#endif

  titleLabel = pgGetWidget(panel, PG_WP_PANELBAR_LABEL);
  if (!titleLabel) {
    /* If we don't have a panelbar, make a toolbar */
    toolbar = pgNewWidget(PG_WIDGET_TOOLBAR,PG_DERIVE_INSIDE,panel);
  }

  /* load the bitmap */
  load_bitmap (argv [1], atoi (argv [2]));
 
  pgEventLoop();
  return 0;
}

/* The End */
