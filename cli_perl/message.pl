#!/usr/bin/perl
use PicoGUI;

$msg = join('',<STDIN>);

$space_sucker = NewWidget(-type => panel,-size => 100,-sizemode =>
4,-bgcolor => 0);

NewWidget(-type => panel,-size => 10,-sizemode => 4,-bgcolor =>
0,-side => top);
NewWidget(-type => panel,-size => 5,-sizemode => 4,-bgcolor =>
0,-side => left);

$label = NewWidget(-type => label, -inside =>$space_sucker, -side => top,
-font => NewFont($ARGV[0],100),-text => NewString($msg),-color =>
0xFFFFFF,-bgcolor => 0,-align => nw);

Update();
while (1) {sleep(1000)};
