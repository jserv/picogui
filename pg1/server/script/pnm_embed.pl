#!/usr/bin/perl
# $Id$
#
# Munge a binary PNM file into a header file to be directly compiled
# into a program
#
# PicoGUI small and efficient client/server GUI
# Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

print "unsigned char ${ARGV[0]}_bits[] = {\n";
while (read STDIN,$c,1) {
    printf("0x%02X, ",ord($c));
    print "\n" if (!((++$i) % 10));
}
print "\n};\n#define ${ARGV[0]}_len $i\n\n";

