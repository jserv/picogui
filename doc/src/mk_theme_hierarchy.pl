#!/usr/bin/perl
# $Id: mk_theme_hierarchy.pl,v 1.2 2000/10/02 04:52:29 micahjd Exp $
#
# This is a small script that uses dot(1) from the
# open source AT&T Graphvis program to create a theme
# hierarchy graph, using the hierarchy table in the
# source code itself.
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
####### configuration

$srcfile = "../../pgserver/theme/memtheme.c";
$postscript = "../ps/themehierarchy.ps";

####### Main program

# Read the thobj_ancestry table from the source file
open SRCF,$srcfile or die "Can't open source file: $!";
while (<SRCF>) {
    last if (/thobj_ancestry/ && /\{/);
}
while (<SRCF>) {
    last if (/\}/);
    next if (!/PGTH_O/);
    if (/PGTH_O_([A-Z_]+)\s*\*\//) {
	($x=$1) =~ tr/A-Z_/a-z./;
	$objs .= "$1 [label=\"$x\"];\n";
    } 
    $cons .= "$1 -> $2;\n" if (/PGTH_O_([A-Z_]+)\s*\*\/\s*PGTH_O_([A-Z_]+)/);
}

# Open a pipe to dot
open DOTF,"| dot -Tps -o $postscript" or die "Can't pipe to dot: $!";
print DOTF <<EOF;
digraph L0 {
    node [shape=box];
    rotate=90;
    $objs $cons
    label = "\\nPicoGUI Theme Object Hierarchy";
    fontsize=18;
}
EOF

### The End ###
