#!/usr/bin/perl
use PicoGUI;

$p = NewWidget(-type => panel);
$chk = NewBitmap(-file => '../images/check.pnm');
$chkm = NewBitmap(-file => '../images/check_mask.pnm');

for ($i=0;$i<10;$i++) {
NewWidget(-type => button, -inside => $p,-bitmap => $chk, -bitmask =>$chkm);
}

Update(); <STDIN>;



