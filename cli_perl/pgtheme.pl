#!/usr/bin/perl
#
# Little utility to send a theme to a PicoGUI server
#
use PicoGUI;

while (<>) {
	s/#.*//;
	next if (!/\S/);
	if (/\s*(\S+)\s*=\s*(\S+)/) {
		if ($1 eq 'background') {
			$bmp = NewBitmap(-file => $2); 
			$bmp->MakeBackground();
		}
		else {
			ThemeSet($1 => eval($2));
		}		
	}
}
