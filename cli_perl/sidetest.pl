#!/usr/bin/perl
use PicoGUI;

RegisterApp(-name => NewString("Button test"),-type => toolbar);

$a = NewString("X");
$b = NewString("Hello, World!");
$w=NewWidget(-type => button,-text => $a,-onclick => sub {
	if ($i) {
		$w->SetWidget(-side=>left);
		$i = 0;
	}
	else {
		$w->SetWidget(-side=>right);
		$i = 1;
	}
	Update();
},
	     -bitmap => NewBitmap(-file=>'../images/button/tux.pnm'),
	     -bitmask => NewBitmap(-file=>'../images/button/tux_mask.pnm'));

NewWidget(-type => label,-text => NewString("This is only a test..."),-font => 
	  NewFont("",10,italic,underline),-color => 0x006666,-transparent=>1,
	  -side=>left);

NewWidget(-type=>button,-onclick => sub{print"click!\n"},
	  -text => NewString("Hi"),-side=>top);


EventLoop;
