/*
 * Test app for the new way to do PicoGUI lists...
 *
 * The list widget is no longer necessary. The functions it was designed
 * to handle can all be accomplished by more general, and therefore more
 * economical, code:
 *
 *  - scrolling
 *    The hotspot system and scrollbar widget provide a good scrolling
 *    system using either the mouse or keyboard
 *
 *  - selection
 *    PG_WP_HILIGHTED now propagates to all children, and button widgets
 *    will set or clear PG_WP_HILIGHTED on their children. Note that the
 *    listitem widget is a type of button.
 *
 *  - traversal
 *    The former list widget had new API calls to retrieve list items by
 *    index. This has been replaced by the very general pgTraverseWidget
 *    API that can be easily used to retrieve a list item's handle given
 *    an index, or find the previous/next item.
 *
 *  - hilight events
 *    Using the listitem widget as the building block for a list, the app
 *    will get a PG_WE_ACTIVATE event when an item is selected.
 *
 *  - columned lists
 *    Using the listitem widget as a container, it's possible to implement
 *    columned lists that can hilight an entire row. The listitem's
 *    container aspect is also useful for creating lists incorporating
 *    interactive elements like buttons or checkboxes.
 *
 * -- Micah
 */

#include <picogui.h>

/* Event handler to print out our hilight/unhilight events
 */
int evtHilight(struct pgEvent *evt) {
  pghandle sText = pgGetWidget(evt->from, PG_WP_TEXT);
  char *text = "(no text)";
  char *evtype;
  if (sText)
    text = pgGetString(sText);

  switch (evt->type) {

  case PG_WE_ACTIVATE: 
    evtype = "PG_WE_ACTIVATE";
    break;

  case PG_WE_PNTR_UP: 
    evtype = "PG_WE_PNTR_UP";
    break;

  default:
    return 0;
  }

  printf("Widget sent %s : %s\n",evtype,text);
  return 0;
}

int main(int argc, char **argv) {
  pghandle wBox, wItem, sFoo;
  int i;

  /* Application
   */
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"List Test App",0);

  /* Scrolling box
   */
  wBox = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgNewWidget(PG_WIDGET_SCROLL, PG_DERIVE_BEFORE, wBox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_BIND, wBox,
	      0);

  /* Add some normal listitems (just text)
   * The listitem widget is hilighted when the cursor is over them,
   * and turned on when clicked. They are mutually exclusive.
   */
  for (i=0;i<5;i++) {
    wItem = pgNewWidget(PG_WIDGET_LISTITEM,
			i ? PGDEFAULT : PG_DERIVE_INSIDE,
			i ? PGDEFAULT : wBox);
    pgReplaceTextFmt(PGDEFAULT,"Normal listitem #%d",i);
  }

  /* Normally you'd want to use listitems, but just to show the
   * difference we'll throw in some menuitems...
   */
  for (i=0;i<5;i++) {
    wItem = pgNewWidget(PG_WIDGET_MENUITEM,0,0);
    pgReplaceTextFmt(PGDEFAULT,"Normal menuitem #%d",i);
  }

  /* Instead of using the PG_WP_TEXT property, create some
   * other widgets inside the listitem. They will be hilighted
   * with their PG_WP_HILIGHTED property when the mouse is over them.
   */
  sFoo = pgNewString("Foo");
  for (i=0;i<5;i++) {
    wItem = pgNewWidget(PG_WIDGET_LISTITEM,PG_DERIVE_AFTER,wItem);

    pgNewWidget(PG_WIDGET_CHECKBOX, PG_DERIVE_INSIDE, wItem);
    pgSetWidget(PGDEFAULT,
		PG_WP_TEXT, sFoo,
		PG_WP_SIDE, PG_S_LEFT,
		PG_WP_SIZEMODE, PG_SZMODE_PERCENT,
		PG_WP_SIZE, 20,
		0);

    pgNewWidget(PG_WIDGET_LABEL, 0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_SIDE, PG_S_LEFT,
		0);
    pgReplaceTextFmt(PGDEFAULT,"Container listitem #%d",i);
  }

  /* For contrast, repeat the same example with menuitems
   */
  for (i=0;i<5;i++) {
    wItem = pgNewWidget(PG_WIDGET_MENUITEM,PG_DERIVE_AFTER,wItem);

    pgNewWidget(PG_WIDGET_CHECKBOX, PG_DERIVE_INSIDE, wItem);
    pgSetWidget(PGDEFAULT,
		PG_WP_TEXT, sFoo,
		PG_WP_SIDE, PG_S_LEFT,
		PG_WP_SIZEMODE, PG_SZMODE_PERCENT,
		PG_WP_SIZE, 20,
		0);

    pgNewWidget(PG_WIDGET_LABEL, 0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_SIDE, PG_S_LEFT,
		0);
    pgReplaceTextFmt(PGDEFAULT,"Container menuitem #%d",i);
  }
  
  /* Just for testing the pgTraverseWidget method of finding
   * an item by its index:
   * (Normally if you wanted to print out all the items in a container
   * you should iterate with PG_TRAVERSE_FORWARD)
   */
  for (i=0;;i++) {
    pghandle w,t;
    char *text = "(no text)";

    w = pgTraverseWidget(wBox, PG_TRAVERSE_CHILDREN, i);
    if (!w)
      break;

    /* Get the widget's text if it has any */
    t = pgGetWidget(w, PG_WP_TEXT);
    if (t)
      text = pgGetString(t);

    printf("Child #%d : %s\n",i, text);
  }
  
  /* Add an event handler to catch all hilighting events
   */
  pgBind(PGBIND_ANY, PGBIND_ANY, evtHilight, NULL);  
  pgEventLoop();
  return 0;
}
