#!/usr/bin/perl
use PicoGUI;

open INFILE,"PicoGUI.pm";
$s = NewString(join '',<INFILE>);

NewPopup(-1,-1,280,200);
$t = NewWidget(-type => label,-text=>$s,-side=>all);
NewWidget(-type => scroll,-bind => $t,-before => $t);

EventLoop;
