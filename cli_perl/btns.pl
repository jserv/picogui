#!/usr/bin/perl
use PicoGUI;

$p = NewWidget(-type => toolbar);
$str = NewString("Click a button");
$lbl = NewWidget(-type => label,-side => top,
		 -text => $str,-before => $p,-bgcolor => 0,-color => 0xFFFFFF);

$check = NewWidget(-type => button, -inside => $p,
	-bitmap  => NewBitmap(-file => '../images/button/check.pnm'),
	-bitmask => NewBitmap(-file => '../images/button/check_mask.pnm'));

$tux = NewWidget(-type => button, -inside => $p,
	-bitmap  => NewBitmap(-file => '../images/button/tux.pnm'),
	-bitmask => NewBitmap(-file => '../images/button/tux_mask.pnm'));

$x = NewWidget(-type => button, -inside => $p,
	-bitmap  => NewBitmap(-file => '../images/button/x.pnm'),
	-bitmask => NewBitmap(-file => '../images/button/x_mask.pnm'));

# Junk button
NewWidget(-type => button,-side => right,-inside => $p);

Update(); 

while (1) {
    ($type,$from,$param) = EventWait();
    
    if ($from==$tux->GetHandle()) {
	$nstr = NewString("You clicked Tux (click #".(++$tuxc).")");
    }
    elsif ($from==$x->GetHandle()) {
	$nstr = NewString("You clicked the X (click #".(++$xc).")");
    }
    elsif ($from==$check->GetHandle()) {
	$nstr = NewString("You clicked the check (click #".(++$checkc).")");
    }
    else {
	$nstr = NewString("Other event");
    }
    
    $lbl->SetWidget(-text => $nstr);
    $str->delete;
    $str = $nstr;
    Update();
}

