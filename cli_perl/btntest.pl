#!/usr/bin/perl
use PicoGUI;

RegisterApp(-name => NewString("Button test"),-type => toolbar);

NewWidget(-type => button,
	  -bitmap => NewBitmap(-file=>'../images/button/tux.pnm'),
	  -bitmask => NewBitmap(-file=>'../images/button/tux_mask.pnm'));

NewWidget(-type => button,-text => $hi = NewString("X"));

NewWidget(-type => button,-text => $hi,-font => 
	  NewFont("",10,italic,underline),-color => 0x006666);

NewWidget(-type => button,-text => $hi,-font=>NewFont("Utopia"),
	  -bitmap => NewBitmap(-file=>'../images/button/x.pnm'),
	  -bitmask => NewBitmap(-file=>'../images/button/x_mask.pnm'));

NewWidget(-type => button,-text => 
	NewString("This is a test..."),
	  -bitmap => NewBitmap(-file=>'../images/button/x.pnm'),
	  -bitmask => NewBitmap(-file=>'../images/button/x_mask.pnm'));

EventLoop;
