#!/usr/bin/perl
#
# Little utility to send a theme to a PicoGUI server
#
use PicoGUI;

while (<>) {
	s/#.*//;
	next if (!/\S/);
	if (/\}/) {
	    pop @prefices;
	}
	elsif (/(\w+)\W*\{/) {
	    push @prefices, $1;
	}
	elsif (/\s*(\S+)\s*=\s*(\S+)/) {
	    if ($1 eq 'background') {
		$bmp = NewBitmap(-file => $2); 
		$bmp->SetBackground();
	    }
	    else {
		 $th = (join('.',@prefices).(@prefices?'.':'').$1);
		 $v = eval($2);
#		 print "$th = $v\n";
		 ThemeSet($th => $v);
	    }		
	}
}


