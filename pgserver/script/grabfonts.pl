#!/usr/bin/perl
# $Id: grabfonts.pl,v 1.6 2001/08/31 05:26:06 micahjd Exp $
#
# This script uses fstobdf to grab fonts from a font server,
# and munge them into fdf files
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

# Make a pass through the font to find the maximum overlap past the edge
# of the character. This will determine how much extra width the character
# cells need, and how much the renderer will move backwards after each
# character. This isn't a great way to handle overlapping characters, but
# until I rework PicoGUI's font system to load BDF data directly or something
# this should work alright.

open BDFF,"fstobdf -s localhost:7100 -fn $xfntname |";
$max_overlap = 0;
while (<BDFF>) {
    if (/^DWIDTH ([0-9]+)/) {
	$deltax = $1;
    }
    elsif (/^ENCODING ([0-9]+)/) {
	$c = $1;
    }
    elsif (/^BBX (\S+) (\S+) (\S+) (\S+)/) {
	$charw = $1 + $3;
	$overlap = $charw - $deltax;

	# Ignore after 128- some fonts do weird things there
	$max_overlap = $overlap if ($overlap > $max_overlap and $c < 128);
    }
}
close BDFF;


# Another pass, to actually output the font data in .fdf format

open BDFF,"fstobdf -s localhost:7100 -fn $xfntname |";

print "# This font was converted from a BDF to FDF using fstobdf\n";
print "# Please see comments below for notices regarding this font\n#\n";
print "[${fstructname}] (-$max_overlap,0) b3\n\n";

while (<BDFF>) {
    chomp;
    if (/^(COMMENT|FONT|COPYRIGHT|NOTICE) (.*)/) {
	print "# $2\n";
    }
    elsif (/^FONT_ASCENT (.*)/) {
	$ascent = $1;
	$celheight = $descent + $ascent;
    }
    elsif (/^FONT_DESCENT (.*)/) {
	$descent = $1;
	$celheight = $descent + $ascent;
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
		$w = $1 + $max_overlap;
	    }
	    elsif (/^BBX (\S+) (\S+) (\S+) (\S+)/) {
		$h = $2;
		$left_pad = $3;
		$top_pad = $ascent - $h - $4;
		$bot_pad = $celheight - $h - $top_pad;
		$hl = 0;
	    }
	    elsif (/^BITMAP/) {
		print "\n: $c\n";
		for ($i=0;$i<$top_pad;$i++) {
		    last if ($hl>=$celheight);
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
      $_ = (". " x $left_pad) . $_;

      $_ = substr($_,0,$w*2);

		# Calculate actual width, we might need padding
		$aw_s = $_;
		$aw_s =~ s/[^\.\#]//g;
		$aw = length $aw_s;

		++$hl;
		if ($hl<=$celheight) {
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
	    last if ($hl>=$celheight);
	    for ($j=0;$j<$w;$j++) {
		print '. ';
	    }
	    print "\n";
	    ++$hl;
	}
    }
}

