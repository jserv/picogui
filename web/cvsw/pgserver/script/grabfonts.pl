#!/usr/bin/perl
# $Id: grabfonts.pl,v 1.1 2000/11/06 00:31:36 micahjd Exp $
#
# This script uses fstobdf to grab fonts from a font server,
# and munge them into fdf files
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

($fname,$fsize,$weight,$slant,$fstructname) = @ARGV;

#$fname = "helvetica";
#$fsize = 24;

#$fname = "utopia";
#$fsize = 25;
#$weight = "regular";
#$slant = "r";

$xfntname = "-*-$fname-$weight-$slant-*-*-$fsize-*-*-*-*-*-*";

# Use the short syntax
if (!$fstructname) {
	($xfntname,$fsize,$fstructname) = @ARGV;
}

open BDFF,"fstobdf -s localhost:7100 -fn $xfntname |";

print "# This font was converted from a BDF to FDF using fstobdf\n";
print "# Please see comments below for notices regarding this font\n#\n";
print "[${fstructname}] (0,1) b3\n\n";


while (<BDFF>) {
    chomp;
    if (/^(COMMENT|FONT|COPYRIGHT|NOTICE) (.*)/) {
	print "# $2\n";
    }
    elsif (/^FONT_DESCENT (.*)/) {
	$descent = $1;
    }
    elsif (/^STARTCHAR (.*)/) {
	$chr = $1;
	$c = "'$chr'";
	while (<BDFF>) {
	    if (/ENDCHAR/) {
		last;
	    }
	    elsif (/^ENCODING ([0-9]+)/) {
		$c = "#".$1;
	    }
	    elsif (/^DWIDTH ([0-9]+)/) {
		$w = $1;
	    }
	    elsif (/^BBX (\S+) (\S+) (\S+) (\S+)/) {
		$h = $2;
		$top_pad = $fsize - $descent - $h - $4;
		$bot_pad = $fsize - $h - $top_pad;
		$hl = 0;
	    }
	    elsif (/^BITMAP/) {
		print "\n: $c\n";
		for ($i=0;$i<$top_pad;$i++) {
		    last if ($hl>=$fsize);
		    for ($j=0;$j<$w;$j++) {
			print '. ';
		    }
		    print "\n";
		    ++$hl;
		}
	    }
	    elsif (!/[^0-9A-Fa-f\s]/) {
		$_ = uc($_);
		s/0/. . . . /g;
		s/1/. . . # /g;
		s/2/. . # . /g;
		s/3/. . # # /g;
		s/4/. # . . /g;
		s/5/. # . # /g;
		s/6/. # # . /g;
		s/7/. # # # /g;
		s/8/# . . . /g;
		s/9/# . . # /g;
		s/A/# . # . /g;
		s/B/# . # # /g;
		s/C/# # . . /g;
		s/D/# # . # /g;
		s/E/# # # . /g;
		s/F/# # # # /g;
		chomp;
		$_ = substr($_,0,$w*2);

		# Calculate actual width, we might need padding
		$aw_s = $_;
		$aw_s =~ s/[^\.\#]//g;
		$aw = length $aw_s;

		++$hl;
		if ($hl<=$fsize) {
			print;	
	
			if ($aw<$w) {
			    for ($i=0;$i<($w-$aw);$i++) {
				print ". ";
		    	}
			}
			print "\n";
		}
	    }
	}
	for ($i=0;$i<$bot_pad;$i++) {
	    last if ($hl>=$fsize);
	    for ($j=0;$j<$w;$j++) {
		print '. ';
	    }
	    print "\n";
	    ++$hl;
	}
    }
}

