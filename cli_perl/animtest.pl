#!/usr/bin/perl
#
# Creates a panel and varies its size. This
# Causes the server to recalc and redraw
# the screen when we Update()
# Kinda neet looking though if you have another
# prog running on the server before you start
# this, because it varies the size available to
# the other prog.
#
use PicoGUI;

$panel = NewWidget(-type => panel,-bgcolor => 0x00FF00);

while (1) {
    for ($i=0;$i<150;$i++) {
	$panel->SetWidget(-size => $i);
	Update();
    }
    for ($i=0;$i<150;$i++) {
	$panel->SetWidget(-size => (150-$i));
	Update();
    }
}
