# $Id: mksymtab.pl,v 1.4 2001/02/08 06:48:28 micahjd Exp $
#
# mksymtab.pl - convert the constant definitions in constants.h
#               into a symbol table for looking up constants
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

print "// Generated by mksymtab.pl from constants.h\n";

while (<>) {
    # Preserve the RCS tag just incase...
    if (/(\$[^\$]+\$)/) {
	print "// $1\n";

	# Start the rest of the file
	print "\n#include \"thdump.h\"\n\nstruct symnode symboltab[] = {\n";

	next;
    }

    # We don't care about the symbol's actual value, let
    # the C compiler sort that out. Just get a list of 'em
    # so we can make a table

    next if (!/^\#define\s*(PG\S+)/);
    $sym = $1;
    next if ($sym =~ /[\(\)]/);   # skip macros 
    print "\t{$sym,\"$sym\"},\n";
}

print "\t{0,NULL}\n};\n\n/* The End */\n";

### The End ###




















