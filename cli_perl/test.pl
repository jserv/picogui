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
$p = NewWidget(-type => panel);

# Show it's handle
print $p->GetHandle()."\n";

# Make a widget that we don't
# need to store. Set params
# while creating it.
NewWidget(-type => scroll,
	  -value => 50);

# Make a panel with the side param
# set
NewWidget(-type => panel,
	  -side => top);

# Set the panel's color
$p->SetWidget(-bgcolor => 0xE0E0C0);

# Make a widget inside the panel
$tx = NewWidget(-type => label,
		-inside => $p,
		-text => NewString("Hi\nA\nB\nC\nD"),
		-transparent => 1);

# Combine some of these techniques and
# make a text box thingy with a bold
$tbox = NewWidget(-type => label,
		  -after => $p,
		  -align => nw,
		  -side => left,
		  -bgcolor => 0x0000FF,
		  -color => 0xFFFF00,
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
$bits = NewBitmap(-file => '/home/micah/images/tux.color.pnm');

# Make a sequence of widgets in a panel
$holder = NewWidget(-type => panel,
		    -side => top,
		    -bgcolor => 0xFFFFFF,
		    -after => $tbox,
		    -size => 40
		    );
for ($i=0;$i<10;$i++) {
    NewWidget(-type => bitmap,
	      -inside => $holder,
	      -side => left,
	      -bitmap => $bits
	      );
}

Update(); <STDIN>;

# Delete something
$tx->delete;
Update(); <STDIN>;




