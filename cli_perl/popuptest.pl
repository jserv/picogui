#!/usr/bin/perl
use PicoGUI;

$w = 150;
$h = 60;
$s = <<EOF;
This is a dialog box?
It has buttons on the side
<--------
EOF

NewPopup($ServerInfo{'Width'}/2-$w/2,$ServerInfo{'Height'}/2-$h/2,$w,$h);
NewWidget(-type => label, -text => NewString("Message Box"),
	  -font => NewFont("",0,bold),-color => 0xFFFFFF,-bgcolor => 0x000000);
$tb = NewWidget(-type => toolbar, -side => left);
NewWidget(-type => label, -text => NewString($s),-align => nw);

NewWidget(-type => button, -onclick => sub {exit 0},
	  -inside => $tb, -side => top,
	  -bitmap => NewBitmap(-file=>'../images/button/check.pnm'),
	  -bitmask => NewBitmap(-file=>'../images/button/check_mask.pnm'));

EventLoop;


