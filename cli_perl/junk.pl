#!/usr/bin/perl
#
# Little program to test all the features of the perl module
#
use PicoGUI;

# Make a widget and store it
for ($i=0;$i<5;$i++) {
NewWidget(-type => panel);
}

Update(); <STDIN>;




