#!/usr/bin/perl
use PicoGUI;

NewPopup(300,220);
NewWidget(-type => label,-transparent => 1,-text => 
	  NewString("Type stuff and it will show up below:"));
$tb = NewWidget(-type => toolbar,-side => bottom);
$l = NewWidget(-type => label,-side => all,-font => NewFont("",0,fixed),
	       -transparent => 0,-text => ($t=NewString("")),
	       -color => 0x8080FF,-bgcolor => 0x000080);
NewWidget(-type => button,-text => NewString("Exit"),-onclick => sub{exit 0},
	  -inside => $tb);
GrabKeyboard(-onchar => \&keysub);
EventLoop;

sub keysub {
    printf "Key code: 0x%08X\n",$c = $_[0];
    $c &= 0xFF;

    if ($c==0xD) {
	$str .= "\n";
    }
    elsif ($c==0x8) {
	substr($str,-1) = '';
    }
    else {
	$str .= pack 'c',$c;
    }

    $n = NewString($str."\0xFE");
    $l->SetWidget(-text => $n);
    $t->delete;
    $t = $n;
    Update();
}
