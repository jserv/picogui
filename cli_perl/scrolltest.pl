#!/usr/bin/perl
use PicoGUI;

$file = "scrolltest.pl";

# Strings
open INFILE,$file;
$s = NewString(join '',<INFILE>);
close INFILE;

# Bitmaps
$tux = NewBitmap(-file => '../images/button/tux.pnm');
$tuxmask = NewBitmap(-file => '../images/button/tux_mask.pnm');
$check = NewBitmap(-file => '../images/button/check.pnm');
$checkmask = NewBitmap(-file => '../images/button/check_mask.pnm');

# Popup
NewPopup(-1,-1,280,200);

# first-level stuff, from top to bottom
NewWidget(-type => label,-font => NewFont("Utopia",0,grayline),-transparent=>1,
	  -text=>NewString("Welcome to PicoGUI"),-side => top);
$box = NewWidget(-type=>box,-size=>0,-bordercolor => 0x003030);
$b1 = NewWidget(-type=>box);

# Stuff in the frame
$t = NewWidget(-type =>label,-text=>$s,-inside => $box,-side=>all,-align=>nw);
NewWidget(-type => scroll,-bind => $t,-before => $t);
NewWidget(-type => label,-font => NewFont("Times",0,bold),-color => 0xFFFFFF,
          -bgcolor => 0x000000,-side => top,-text => NewString($file),
	  -before => $t);

# b1
NewWidget(-type => label,-transparent => 1,-side => left,-text =>
	  NewString("Click a neat button:"),-inside => $b1);
$vs = NewString('View Source');
$hs = NewString('Hide Source');
$srcbtn = NewWidget(-type => button,-bitmap => $tux,-bitmask => $tuxmask,
	  -text => NewString('View Source'),-onclick => sub {
	      if ($i) {
		  $srcbtn->SetWidget(-text => $vs);
		  $box->SetWidget(-size => 0);
		  $i = 0;
	      }
	      else {
		  $srcbtn->SetWidget(-text => $hs);
		  $box->SetWidget(-size => 100);
		  $i = 1;
	      }
	      Update();
});
NewWidget(-type => button,-font => NewFont("",0,fixed),
	  -text => NewString("Hello\nWorld"),-onclick=>sub {
	      print "Hello, World!\n";
});
NewWidget(-type => button,-bitmap => $check,-bitmask => $checkmask,
	  -onclick => sub {exit 0});

EventLoop;
