#!/usr/bin/perl
use PicoGUI;

NewPopup(30,30,100,100);
NewWidget(-type => label, -text => NewString("Simulation inputs:"),
	  -color => 0xFFFFFF,-bgcolor => 0x000000);
NewWidget(-type => button, -side => bottom, -onclick => sub{exit 0});
NewWidget(-type => scroll,-onchange => sub {$i->SetWidget(-value=>100-$_[0])});
$i = NewWidget(-type => indicator,-side => all,-value=>100);

EventLoop;
