#!/usr/bin/perl
use PicoGUI;

while (1) {
	$x = int(rand($ServerInfo{'Width'}));
	$y = int(rand($ServerInfo{'Height'}));

	SendPoint(1<<(8+int(rand(3))),$x,$y,1);

	SendPoint(1<<(8+int(rand(3))),
		$x+int(rand(10))-5,$y+int(rand(10))-5,0);
}
