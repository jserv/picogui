#!/usr/bin/perl
# $Id$
#
# This script turns a directory full of .fi and .bdf font files
# into C source code that is compiled into the PicoGUI server.
# The .fi files define a fontstyle, including one or more .bdf
# font files. The .bdf files are parsed (using the word loosely :)
# and converted into fonts and fontglyphs. This program now translates
# directly from BDF to C, without using the old FDF format.
#
# Yes, I know this code is messy...
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

print <<EOF;
#include <pgserver/common.h>
#include <pgserver/font_bdf.h>

EOF

# Find which fonts we need from .config
$conffile = shift or die "gimme a config file !";
$fontdir  = shift or ".";

open CONFFILE, $conffile or die "Can't open $conffile: $!";
while (<CONFFILE>) {
      next if (/#/);
      if (/FONT_([\w]+)/) {
      	 push @fontfiles, lc($1).".fi";
      }
}
close CONFFILE;

foreach $file (@fontfiles) {
    open FIFILE, "$fontdir/$file" or die "Can't open the font '$file': $!";
    %fiparam = ();
    while (<FIFILE>) {
	chomp;
	($key,$val) = split /\s*=\s*/;
	$fiparam{$key} = $val;
    }
    close FIFILE;
    # Now we have all the params in memory
   
    # default bold width is 1
    $fiparam{'BOLDW'} = 1 if (!$fiparam{'BOLDW'});
    $fiparam{'STYLE'} = join('|',map('PG_FSTYLE_'.$_,
				     split(/\s/,uc($fiparam{'STYLE'}))));
    $norm = $bold = $ital = $bital = 'NULL';
    $fiparam{'STYLE'} = 0 if (!$fiparam{'STYLE'});

    if ($fiparam{'NORMAL'}) {
	$norm = '(struct bdf_font *) &'.$fiparam{'NORMAL'};
	$fdfs{$fiparam{'NORMAL'}} = 1;
    }
    if ($fiparam{'BOLD'}) {
	$bold = '(struct bdf_font *) &'.$fiparam{'BOLD'};
	$fdfs{$fiparam{'BOLD'}} = 1;
    } 
    if ($fiparam{'ITALIC'}) {
	$ital = '(struct bdf_font *) &'.$fiparam{'ITALIC'};
	$fdfs{$fiparam{'ITALIC'}} = 1;
    } 
    if ($fiparam{'BOLDITALIC'}) {
	$bital = '(struct bdf_font *) &'.$fiparam{'BOLDITALIC'}; 
	$fdfs{$fiparam{'BOLDITALIC'}} = 1;
    }

    $node = "\"".$fiparam{'NAME'}."\",".$fiparam{'SIZE'}.",".
	$fiparam{'STYLE'}.",###,$norm,$bold,$ital,$bital,".$fiparam{'BOLDW'};

    push @defs, $node;
}

# Build the font datas
foreach $fntname (sort keys %fdfs) {
    open FDATA,"$fontdir/$fntname.bdf";

    $numglyphs = 0;
    $defaultglyph = 32;
    $h = 0;
    $ascent = 0;
    $descent = 0;
    
    @glyphs = ();
    $bitmapdata = "";

    # ************ Parse BDF file
    while (<FDATA>) {
	chomp;

	# Top-level lines (for the whole font, not a glyph)
	
	if (/^DEFAULT_CHAR\s*(\S+)/) {
	    $defaultglyph = $1;
	}
	elsif (/^PIXEL_SIZE\s*(\S+)/) {
	    $h = $1;
	}
	elsif (/^FONT_ASCENT\s*(\S+)/) {
	    $ascent = $1;
	}
	elsif (/^FONT_DESCENT\s*(\S+)/) {
	    $descent = $1;
	}
	elsif (/^FONTBOUNDINGBOX\s*(\S+)\s+(\S+)\s+(\S+)\s+(\S+)/) {
	    $fw = $1;
	    $fh = $2;
	    $fx = $3;
	    $fy = $4;
	}
	elsif (/^STARTCHAR/) {

	    # Process one glyph

	    $defaultglyph = 32 if (!$defaultglyph);

	    $bitmapmode = 0;
	    while (<FDATA>) {
		chomp;

		if ($bitmapmode and !/[^0-9a-fA-F]/) {
		    $bitmapdata .= $_;
		}
		elsif (/^ENCODING\s*(\S+)/) {
		    $encoding = $1;
		    $gdwidth = $gw = $gh = $gx = $gy =  0;
		    $gbitmap = length($bitmapdata)/2;
		}
		elsif (/^ENDCHAR/) {
		    # If this character had no actual bitmap data, and isn't the default glyph,
                    # omit it. Some fonts include useless duplicates of the default glyph that just
		    # take up space before or after the actual characters.
		    if (length($bitmapdata)/2 == $gbitmap and $encoding != $defaultglyph) {
		    }
		    else {
			$numglyphs++;
			push @glyphs, "encoding: $encoding, bitmap: $gbitmap, dwidth: $gdwidth, w: $gw, h: $gh, x: $gx, y: $gy";
		    }
		}
		elsif (/^DWIDTH\s*(\S+)/) {
		    $gdwidth = $1;
		}
		elsif (/^BBX\s*(\S+)\s+(\S+)\s+(\S+)\s+(\S+)/) {
		    $gw = $1;
		    $gh = $2;
		    $gx = $3;
		    $gy = $4;
		}
		elsif (/^BITMAP/) {
		    $bitmapmode = 1;
		}
		
	    }
	}
    }
    
    # ************ Output glyph table

    print "\nstruct bdf_fontglyph const ${fntname}_glyphs[$numglyphs] = {\n";
    foreach (@glyphs) {
	print "{$_},\n";
    }
    print "};\n";
    
    # ************ Output bitmap data
    
    print "\nunsigned char const ${fntname}_bits[] = {\n";
    for ($i=0; $i<length($bitmapdata); $i+=2) {
	print "0x".substr($bitmapdata,$i,2).", ";
	print "\n" if (($i&15)==14);
    }
    print "};\n";

    # ************ Font structure
    
    print "\nstruct bdf_font const $fntname = {\n\t";
    print "numglyphs: $numglyphs, defaultglyph: $defaultglyph, w: $fw,\n";
    print "h: $h, ascent: $ascent, descent: $descent,\n"; 
    print "glyphs: ${fntname}_glyphs, bitmaps: ${fntname}_bits\n";
    print "\n};\n";
    
#    $hdrs = 256*5;
#    $totalsize = $hdrs+$size;

#    print STDERR "Summary of font [$fntname] :\n";
#    print STDERR "\theaders = $hdrs\n";
#    print STDERR "\tchardata = $size\n";
#    print STDERR "\tnumchars = $num\n";
#    print STDERR "\ttotalsize = $totalsize\n";

    close FDATA;
}

# Link up the fontstyle nodes and print them out
$link = 'NULL';
$fnode = 0;
foreach (sort @defs) {
    $fnode++;
    print "struct bdf_fontstyle_node const fsn$fnode = {\n";
    s/###/$link/;
    print "$_ };\n";
    $link = "(struct bdf_fontstyle_node *) &fsn$fnode";
}

print "struct bdf_fontstyle_node *bdf_fontstyles = $link;\n/* The End */\n";

# The End #
