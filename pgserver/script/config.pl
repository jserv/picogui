#!/usr/bin/perl
# $Id: config.pl,v 1.9 2000/08/27 05:54:27 micahjd Exp $
#
# Configuration options for PicoGUI.
# This creates the .config file that Makefile uses.
# Requires the 'dialog' command.
#
# PicoGUI small and efficient client/server GUI
# Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
# 
# Contributors:
#
#
#
#

%optdesc = (
	    'DEBUG' => 'Debugging option',
	    'NET' => 'Network type',
	    'FONTSET' => 'Font set',
	    'STRIPBIN' => 'Strip binary',
	    'INPUT' => 'Input driver',
	    'PLATFORM' => 'Target platform'
	    );

%opts = (
	 'DEBUG' => 'none',
	 'NET' => 'socket',
	 'FONTSET' => 'all',
	 'STRIPBIN' => 'no',
	 'INPUT' => 'sdl',
	 'PLATFORM' => 'linux'
	 );

%optopts = (
	    'DEBUG' => 'tiny=Numerical errors:'.
		       'none=Verbose errors, but no debugging:'.
	               'src=Source code DEBUG flag:'.
	               'efence=ElectricFence malloc debug:'.
	               'gcov=Coverage analysis:'.
	               'gprof=Profiling',
	    'NET' => 'socket=Unix-style TCP sockets',
	    'FONTSET' => 'all=All fonts:'.
	                 'minimal=Helvetica:'.
	                 'fixed=Helvetica, Console:'.
	                 'standard=Helvetica, Console, Times, Tiny',
	    'STRIPBIN' => 'no=Leave debugging symbols:'.
	               'yes=Zap debugging symbols',
	    'INPUT' => 'sdl=Input from SDL event loop:'.
	               'null=No input driver (network only)',
	    'PLATFORM' => 'linux=Compile natively for linux:'.
		          'windows=Cross-compile for M$ windoze'
	    );

########

$DIALOGCMD = 'dialog --title "PicoGUI Config" ';

system "$DIALOGCMD --msgbox \"".<<EOF."\" 20 50";
Welcome to PicoGUI configuration.  This
program allows you to select driver options
and debugging options for pgserver. It 
creates a file, '.config', in the current
directory, which is used by the Makefile to
determine compile-time options.

This configuration can be invoked in the
future with the command 'make config'

Micah Dowty <micah\@homesoftware.com>
EOF

if (open CNFFILE,".config") {
    $r = system "$DIALOGCMD --yesno \"Use existing .config as defaults?\"".
	" 4 50";
    if (!$r) {
	# Read existing options
	while (<CNFFILE>) {
	    s/\#.*//;
            while (s/\s$//) {}
	    next if (!/^\s*(\S+)\s*=\s*(.*)/);
	    $opts{$1} = $2;
	}
    }
    close CNFFILE;
}

while (1) {
    
    # Build the menu
    %menus = ();
    foreach (keys %optdesc) {
	$menus{$optdesc{$_}} = $opts{$_};
	$key{$optdesc{$_}} = $_;
    }
    $menus{'Save'} = '';
    
    # Choose an option...
    system "$DIALOGCMD --menu \"Select an option:\" 20 60 15 \"".
	join("\" \"",%menus)."\" 2> tmp.choice";
    open CHOICE,'tmp.choice' or die $!;
    $r = <CHOICE>;
    chomp $r;
    close CHOICE;
    unlink 'tmp.choice';

    if ($r eq 'Save') {
	open CNFOUT,">.config";
	foreach (keys %opts) {
	    print CNFOUT "$_ = $opts{$_}\n";
	}
	close CNFOUT;
	system "$DIALOGCMD --msgbox \"Configuration Saved\" 6 50";
	exit(0);
    }

    $key = $key{$r};
    exit 1 if (!$key);

    # Now choose the value of that option
    %menus = ();
    $op = $optopts{$key};
    foreach (split /:/,$op) {
	($opt,$desc) = split /=/;
	$menus{$opt} = $desc;
    }
    # Run the menu
    system "$DIALOGCMD --menu \"Select a value for $key:\" 20 60 15 \"".
	join("\" \"",%menus)."\" 2> tmp.choice";
    open CHOICE,'tmp.choice' or die $!;
    $r = <CHOICE>;
    chomp $r;
    close CHOICE;
    unlink 'tmp.choice';
	
    # Set it
    $opts{$key} = $r if ($r);
}

### The End ###










