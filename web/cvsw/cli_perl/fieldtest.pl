#!/usr/bin/perl
use PicoGUI;

NewPopup(200,300);
NewWidget(-type => label,-text => 
	  NewString("PicoGUI field test"),-bgcolor => 0x000000,
	  -color => 0xFFFFFF);
$tb = NewWidget(-type => toolbar,-side => bottom);

$box = NewWidget(-type => box);
$box2 = NewWidget(-type => box);

NewWidget(-type => label,-transparent => 1,-text =>
	  NewString("Type stuff:"),-side => left,-inside => $box);
NewWidget(-type => field, -text => NewString("Initial text"),
	  -side => all);

NewWidget(-type => label,-transparent => 1,-text =>
	  NewString("Type more:"),-side => left,-inside => $box2);
NewWidget(-type => field, -text => NewString("blah"),
	  -side => all);

NewWidget(-type => button,-text => NewString("Exit"),-onclick => sub{exit 0},
	  -inside => $tb);
EventLoop;






