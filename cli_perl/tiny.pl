#!/usr/bin/perl
use PicoGUI;

popbox(int(rand(300)),int(rand(200)),50,30);


sub popbox {
    my ($x,$y,$w,$h) = @_;
    NewPopup($x,$y,$w,$h);
    NewWidget(-type => label, -text =>
	NewString("Hello!"),-side=>all,-transparent=>1);
    Update();
}    

EventLoop;
