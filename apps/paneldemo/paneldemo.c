/*
 * Demonstration on how to customize the panel and the panelbar
 */

#include <picogui.h>

/* Arrow bitmap: 11x6 1bpp PBM file */
unsigned char arrow_bits[] = {
0x50, 0x34, 0x0A, 0x0A, 0x31, 0x31, 0x20, 0x36, 0x0A, 
0xFF, 0xE0, 
0x7F, 0xC0, 
0x3F, 0x80, 
0x1F, 0x00, 
0x0E, 0x00, 
0x04, 0x00, 
};
#define arrow_len 21

int btnMenu(struct pgEvent *evt) {
  pgMenuFromString("Foo|Bar|Ecky|Ni");
  return 0;
}

int main(int argc, char **argv) {
  pghandle wPanel, wZoom, wRotate, wClose, wPanelBar, wLabel;
  pghandle bArrow, bArrowMask;
  pghandle wSizableBox, wMyPanelBar;

  /* Connect to the PicoGUI server and create an app as normal
   */

  pgInit(argc,argv);
  wPanel = pgRegisterApp(PG_APP_NORMAL,"Custom Panel Demo",0);

  /* We can use widget properties to obtain handles to all
   * the important sub-widgets inside the panel. These are all
   * normal widgets (buttons, a label, and a panelbar) created
   * automatically by the panel widget.
   */
  
  wPanelBar = pgGetWidget(wPanel, PG_WP_PANELBAR);
  wLabel    = pgGetWidget(wPanel, PG_WP_PANELBAR_LABEL);
  wZoom     = pgGetWidget(wPanel, PG_WP_PANELBAR_ZOOM);
  wRotate   = pgGetWidget(wPanel, PG_WP_PANELBAR_ROTATE);
  wClose    = pgGetWidget(wPanel, PG_WP_PANELBAR_CLOSE);

  /* Let's say we don't want a zoom button on this window...
   * We can just delete it.
   */

  pgDelete(wZoom);

  /* We can also modify the builtin widgets. This makes the
   * close button twice its normal size.
   */

  pgSetWidget(wClose,
	      PG_WP_SIZE, 2 * pgThemeLookup(PGTH_O_CLOSEBTN,PGTH_P_WIDTH),
	      0);

  /* The title is a widget too. This left-justifies the title and leaves
   * room next to it for more widgets. Note that the side parameters
   * used here should be consistent with the initial orientation of your
   * application (usually a horizontal panelbar) but they will be
   * automatically changed if the app is rotated.
   */

  pgSetWidget(wLabel,
	      PG_WP_SIDE, PG_S_LEFT,
	      0);

  /* Since the label widget is just a customized button, it can also
   * display bitmaps. Here we give our title a tux icon.
   */
  
  pgSetWidget(wLabel,
	      PG_WP_BITMAP, pgNewBitmap(pgFromFile("tux.pnm")),
	      PG_WP_BITMASK, pgNewBitmap(pgFromFile("tux_mask.pnm")),
	      0);

  /* We can also add any picogui widget to the panelbar. Here we add
   * an indicator widget. You will notice that any non-interactive
   * widget, like this indicator, can be used to drag the panelbar.
   */

  pgNewWidget(PG_WIDGET_INDICATOR, PG_DERIVE_BEFORE, wLabel);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_RIGHT,
	      PG_WP_SIZEMODE, PG_SZMODE_PERCENT,
	      PG_WP_SIZE, 30,
	      PG_WP_VALUE, 60,
	      0);
	     
  /* Interactive widgets like buttons will of course work fine.
   * A popular use for the new panelbar may be to add menus.
   * Here we create a menu button with the arrow bitmap and
   * event handler above.
   */

  bArrowMask = pgNewBitmap(pgFromMemory(arrow_bits,arrow_len));
  bArrow = pgCreateBitmap(11,6);
  pgRender(bArrow,PG_GROP_SETCOLOR,0x000000);
  pgRender(bArrow,PG_GROP_RECT,0,0,15,8);

  pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_BEFORE, wLabel);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_LEFT,
	      PG_WP_BITMAP, bArrow,
	      PG_WP_BITMASK, bArrowMask,
	      PG_WP_EXTDEVENTS, PG_EXEV_PNTR_DOWN,
	      0);
  pgBind(PGDEFAULT, PG_WE_PNTR_DOWN, btnMenu, NULL);

  /* Another nifty feature of the new panel, is that the panelbar
   * is now a completely self-contained widget. The panelbar widget
   * is very versatile- whenever it is dragged, it simply applies
   * the same change in size to the widget it's bound to. It's designed
   * to be used with a container widget that the panelbar is inside:
   */
  
  wSizableBox = pgNewWidget(PG_WIDGET_BOX, PG_DERIVE_INSIDE, wPanel);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_RIGHT,
	      PG_WP_SIZEMODE, PG_SZMODE_PERCENT,
	      PG_WP_SIZE, 50,
	      PG_WP_TRANSPARENT, 1,
	      PG_WP_MARGIN, 0,
	      0);

  wMyPanelBar = pgNewWidget(PG_WIDGET_PANELBAR, PG_DERIVE_INSIDE, wSizableBox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_LEFT,
	      PG_WP_BIND, wSizableBox,
	      0);

  /* Add some labels so we can tell what's in and out of our sizable container
   */

  pgNewWidget(PG_WIDGET_LABEL, PG_DERIVE_AFTER, wMyPanelBar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_ALL,
	      PG_WP_TEXT, pgNewString("Inside\nwSizableBox"),
	      0);

  pgNewWidget(PG_WIDGET_LABEL, PG_DERIVE_AFTER, wSizableBox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_ALL,
	      PG_WP_TEXT, pgNewString("Outside\nwSizableBox"),
	      0);

  /* Add a label to our panel bar. This illustrates how to make it look like
   * the panel's label widget.
   */

  pgNewWidget(PG_WIDGET_LABEL, PG_DERIVE_INSIDE, wMyPanelBar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_ALL,
	      PG_WP_DIRECTION, PG_DIR_VERTICAL,
	      PG_WP_THOBJ, PGTH_O_PANELBAR,
	      PG_WP_TEXT, pgNewString("wMyPanelBar"),
	      0);

  /* Process events... 
   * The only event we need to process is the menu button
   */

  pgEventLoop();
  return 0;
}
