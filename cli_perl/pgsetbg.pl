#!/usr/bin/perl
#
# A small program to set the background bitmap of a PicoGUI server
# to a PNM file specified on the command line.  Calling with no
# filename args will restore the background to its default.
# $Revision: 1.1 $
#
# Micah Dowty <micah@homesoftware.com>
#
# This file is released under the GPL. Please see the file COPYING that
# came with this distribution.
#
use PicoGUI;

if ($ARGV[0]) {
    print "Setting background to '$ARGV[0]'\n";
    $bmp = NewBitmap(-file => $ARGV[0]); 
    $bmp->MakeBackground();
}
else {
    print "Restoring background\n";
    RestoreBackground();
}
Update();

# The End #
