#!/usr/bin/perl
#
# Little program to test all the features of the perl module
#
use PicoGUI;

# List server information
foreach (keys %ServerInfo) {
    print "$_ = $ServerInfo{$_}\n";
}

# Make a widget and store it
$p = NewWidget(-type => toolbar);

# Show it's handle
print $p->GetHandle()."\n";

# Make a widget that we don't
# need to store. Set params
# while creating it.
NewWidget(-type => scroll,
	  -value => 50);

# Make a toolbar with the side param
# set
$topbar = NewWidget(-type => toolbar,
	    -side => bottom);

# Make a widget inside the toolbar
$tx = NewWidget(-type => label,
		-inside => $p,
		-text => NewString("This text brought you you by test.pl"),
		-transparent => 1,-side => left);
NewWidget(-type => button,-before => $tx,
	  -bitmap => NewBitmap(-file=>'../images/button/check.pnm'),
	  -bitmask => NewBitmap(-file=>'../images/button/check_mask.pnm'));

# Combine some of these techniques and
# make a text box thingy with a bold font
$tbox = NewWidget(-type => label,
		  -after => $p,
		  -align => nw,
		  -side => left,
		  -bgcolor => 0x000080,
		  -color => 0xFFFFFF,
		  -font => NewFont("",0,bold,grayline),
		  -text => NewString(<<EOF));
This is text!
Hello.......
Testing
    1
    2
    3
EOF

# Load a bitmap from a file
$bits = NewBitmap(-file => '../images/other/tux.pnm');

# Make a sequence of widgets in a toolbar
$holder = NewWidget(-type => toolbar,
		    -side => top,
		    -after => $tbox,
		    );
for ($i=0;$i<10;$i++) {
    NewWidget(-type => bitmap,
	      -inside => $holder,
	      -side => left,
	      -bitmap => $bits
	      );
}

# Buttons
for ($i=0;$i<10;$i++) {
	NewWidget(-type => button, -inside => $topbar);
}

Update(); <STDIN>;




