#!/usr/bin/perl
#
# Small perl picogui app to make the theme thumbnails
#

use PicoGUI;

unlink "themename"; 
$s = pgThemeLookup(PGTH_O_DEFAULT,PGTH_P_NAME);
open NAMEF,">themename";
if ($s) {
  print NAMEF pgGetString($s)."\n";
}
close NAMEF;

pgRegisterApp(PG_APP_NORMAL,"App");
pgNewWidget(PG_WIDGET_SCROLL);
$t = pgNewWidget(PG_WIDGET_TOOLBAR);
pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_BOTTOM);
pgNewWidget(PG_WIDGET_INDICATOR);
pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_BOTTOM,PG_WP_VALUE,70);
pgNewWidget(PG_WIDGET_LABEL);
pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Label"),PG_WP_SIDE,PG_S_BOTTOM);

pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,$t);
pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Button"));

pgUpdate();
