#!/usr/bin/perl
use PicoGUI;

$_ = join('',<>);
chomp;
$s = NewString($_);
$f = NewFont("",0,fixed);

NewPopup(-1,-1,GetTextSize($s,$f));
NewWidget(-type => label,-text=>$s,-font=>$f,
	-side=>all,-transparent=>1);

EventLoop;
