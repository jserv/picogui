# $Id: mksymtab.pl,v 1.10 2002/01/05 16:51:46 micahjd Exp $
#
# mksymtab.pl - convert the constant definitions in constants.h
#               into a symbol table to compile into the theme
#               compiler
#
# PicoGUI small and efficient client/server GUI
# Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

$headers = '#include "'.join("\"\n#include \"",@ARGV).'"'; 

print <<EOF;
// Generated by mksymtab.pl from PicoGUI header files

#include "themec.h"
#include "pgtheme.h"

$headers

struct symnode symboltab[] = {
EOF

while (<>) {

    # We don't care about the symbol's actual value, let
    # the C compiler sort that out. Just get a list of 'em
    # so we can make a table

    next if (!/^\#define\s*(PG\S+)/);
    $n = $1;
    next if ($n =~ /\(/);

    # All values can be used as-is as a numerical constant
    $index++;
    print "\t{NUMBER,\"$n\",$n,\&symboltab[$index]},\n";

    # If this is a thobj, put it in with dotted lowercase notation
    if ($n =~ /^PGTH_O_(.*)/) {
	$_ = $1;
	tr/A-Z_/a-z./;
	$index++;
	print "\t{THOBJ,\"$_\",$n,\&symboltab[$index]},\n";
    }

    # Same thing for properties
    if ($n =~ /^PGTH_P_(.*)/) {
	$_ = $1;
	tr/A-Z_/a-z./;
	$index++;
	print "\t{PROPERTY,\"$_\",$n,\&symboltab[$index]},\n";
    }

    # Translate gropnodes to fillstyle functions
    if ($n =~ /^PG_GROP_(.*)/) {
	$_ = $1;
	tr/A-Z/a-z/;
	s/(set)(.)/$1.uc($2)/e;
	s/^(.)/uc($1)/e;
	$index++;
	print "\t{FSFUNC,\"$_\",$n,\&symboltab[$index]},\n";
    }
}

print "\t{NUMBER,\"NULL\",0,NULL}\n};\n\n/* The End */\n";

### The End ###





















