#!/usr/bin/perl
# $Id: fontdef.pl,v 1.19 2001/10/26 23:56:32 micahjd Exp $
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
#include <pgserver/font.h>

EOF

# Find which fonts we need from .config
$conffile = shift or die "gimme a config file !";
$fontdir  = shift or ".";

open CONFFILE, $conffile or die "Can't open $conffile: $!";
while (<CONFFILE>) {
      next if (/#/);
      if (/FONT_([^\s=]+)/) {
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
	$norm = '(struct font *) &'.$fiparam{'NORMAL'};
	$fdfs{$fiparam{'NORMAL'}} = 1;
    }
    if ($fiparam{'BOLD'}) {
	$bold = '(struct font *) &'.$fiparam{'BOLD'};
	$fdfs{$fiparam{'BOLD'}} = 1;
    } 
    if ($fiparam{'ITALIC'}) {
	$ital = '(struct font *) &'.$fiparam{'ITALIC'};
	$fdfs{$fiparam{'ITALIC'}} = 1;
    } 
    if ($fiparam{'BOLDITALIC'}) {
	$bital = '(struct font *) &'.$fiparam{'BOLDITALIC'}; 
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
    $beginglyph = -1;
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
		    if ($encoding+1 != $1 and $beginglyph!=-1) {
			# We will later replace these extras with copies of the default glyph
			for ($i=$1-$encoding-1;$i;$i--) {
			    $numglyphs++;
			    push @glyphs, "0,0,0,0,0,0";

			}
		    }
		    $encoding = $1;
		    $saved_beginglyph = $beginglyph;
		    $beginglyph = $encoding if ($beginglyph==-1);
		    $gdwidth = $gw = $gh = $gx = $gy =  0;
		    $gbitmap = length($bitmapdata)/2;
		}
		elsif (/^ENDCHAR/) {
		    # If this character had no actual bitmap data, and isn't the default glyph,
                    # omit it. Some fonts include useless duplicates of the default glyph that just
		    # take up space before or after the actual characters.
		    if (length($bitmapdata)/2 == $gbitmap and $encoding != $defaultglyph) {
			$beginglyph = -1 if ($saved_beginglyph==-1);
		    }
		    else {
			$numglyphs++;
			push @glyphs, "$gdwidth, $gw, $gh, $gx, $gy, $gbitmap";
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

    print "\nstruct fontglyph const ${fntname}_glyphs[$numglyphs] = {\n";
    $i = $beginglyph;
    foreach (@glyphs) {
	$_ = $glyphs[$defaultglyph-$beginglyph] if ($_ eq "");
	print "{$_},\t/* $i */ \n";
	$i++;
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
    
    print "\nstruct font const $fntname = {\n\t";
    print "$numglyphs, $beginglyph, $defaultglyph, $fw, $h, $ascent, ";
    print "$descent, ${fntname}_glyphs, ${fntname}_bits\n";
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
    print "struct fontstyle_node const fsn$fnode = {\n";
    s/###/$link/;
    print "$_ };\n";
    $link = "(struct fontstyle_node *) &fsn$fnode";
}

print "struct fontstyle_node *fontstyles = $link;\n/* The End */\n";

# The End #
