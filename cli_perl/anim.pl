#!/usr/bin/perl
use PicoGUI;

RegisterApp(-name => NewString("Animated thingy"),
	-type => toolbar);

$i = NewWidget(-type => indicator);

while (1) {
	for ($v=0;$v<=100;$v++) {
		$i->SetWidget(-value => $v);
		Update();
	}
}
