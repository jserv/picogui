#!/usr/bin/perl
use PicoGUI;

RegisterApp(-name => "Toolbar test",-type => toolbar);

NewWidget(-type => label,-side => left,
	  -text => NewString("Click tux: "),-transparent => 1,
	  -font => NewFont("Helvetica",0,bold));

NewWidget(-type => button, -onclick => \&tuxclick,
	  -bitmap => NewBitmap(-file=>'../images/button/tux.pnm'),
	  -bitmask => NewBitmap(-file=>'../images/button/tux_mask.pnm'));

EventLoop;

sub tuxclick {
    printf "Hello World!\n";
}
