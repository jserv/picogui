#!/usr/bin/perl
use PicoGUI;

NewPopup(100,100,100,30);

NewWidget(-type => button, -onclick => sub {exit 0},-side => left);

NewWidget(-type => label, -text => NewString("Hello"),-align => nw,
	  -transparent => 1,-font => NewFont("",30));
    
EventLoop;
