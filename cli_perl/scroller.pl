#!/usr/bin/perl
use PicoGUI;

$p = NewWidget(-type => toolbar);
$str = NewString("Click a button to manipulate the scroll bar");
$lbl = NewWidget(-type => label,-side => top,
		 -text => $str,-before => $p,-bgcolor => 0,-color
		=>0xFFFFFF);

$x = NewWidget(-type => button, -inside => $p,
	-bitmap  => NewBitmap(-file => '../images/button/x.pnm'),
	-bitmask => NewBitmap(-file => '../images/button/x_mask.pnm'));

$check = NewWidget(-type => button, -inside => $p,
	-bitmap  => NewBitmap(-file => '../images/button/check.pnm'),
	-bitmask => NewBitmap(-file => '../images/button/check_mask.pnm'));

$sc = NewWidget(-type => scroll);

Update(); 

while (1) {
    ($type,$from,$param) = EventWait();
    
    if ($from==$x->GetHandle()) {
	$n-=5;
    }
    elsif ($from==$check->GetHandle()) {
	$n+=5;
    }
    
    $sc->SetWidget(-value => $n);
    $nstr = NewString($n."%");
    $lbl->SetWidget(-text => $nstr);
    $str->delete;
    $str = $nstr;
    Update();
}

