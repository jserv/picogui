#!/usr/bin/perl
use PicoGUI;

RegisterApp(-name => NewString("Toolbar test"),-type => toolbar);

NewWidget(-type => label,-side => left,
	  -text => NewString("Click tux: "),-transparent => 1,
	  -font => NewFont("Helvetica",0,bold));

NewWidget(-type => button, -onclick => \&tuxclick,
	  -bitmap => NewBitmap(-file=>'../images/button/tux.pnm'),
	  -bitmask => NewBitmap(-file=>'../images/button/tux_mask.pnm'));

EventLoop;

sub tuxclick {

    $w = 150;
    $h = 60;
    $s = "Hello, World!\nYou clicked tux.";

    $p = NewPopup($ServerInfo{'Width'}/2-$w/2,$ServerInfo{'Height'}/2-$h/2,$w,$h);
    NewWidget(-type => label, -text => NewString("Message Box"),
	      -font => NewFont("",0,bold),-color => 0xFFFFFF,-bgcolor => 0x000000);
    $tb = NewWidget(-type => toolbar, -side => left);
    NewWidget(-type => label, -text => NewString($s),-align => nw,
	      -transparent => 1);
    
    NewWidget(-type => button, -onclick => sub {$p->delete},
	      -inside => $tb, -side => top,
	      -bitmap => NewBitmap(-file=>'../images/button/check.pnm'),
	      -bitmask => NewBitmap(-file=>'../images/button/check_mask.pnm'));
}
