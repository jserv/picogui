#!/usr/bin/perl
# $Id: fontdef.pl,v 1.5 2000/10/10 00:33:37 micahjd Exp $
#
# This reads in .fi files, and creates the static linked list
# of font styles.  It also uses cnvfont to load the .fdf files
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

print "#include <pgserver/font.h>\n#define NULL 0\n\n";

foreach $file (@ARGV) {
    open FIFILE,$file or die $!;
    %fiparam = ();
    while (<FIFILE>) {
	chomp;
	($key,$val) = split /\s*=\s*/;
	$fiparam{$key} = $val;
    }
    close FIFILE;
    # Now we have all the params in memory
    
    $fiparam{'GEO'} = join(',',split(/\s/,$fiparam{'GEO'}));
    $fiparam{'STYLE'} = join('|',map('PG_FSTYLE_'.$_,
				     split(/\s/,uc($fiparam{'STYLE'}))));
    $norm = $bold = $ital = $bital = 'NULL';

    if ($fiparam{'NORMAL'}) {
	$norm = '&'.$fiparam{'NORMAL'};
	$fdfs{$fiparam{'NORMAL'}} = 1;
    }
    if ($fiparam{'BOLD'}) {
	$bold = '&'.$fiparam{'BOLD'};
	$fdfs{$fiparam{'BOLD'}} = 1;
    } 
    if ($fiparam{'ITALIC'}) {
	$ital = '&'.$fiparam{'ITALIC'};
	$fdfs{$fiparam{'ITALIC'}} = 1;
    } 
    if ($fiparam{'BOLDITALIC'}) {
	$bital = '&'.$fiparam{'BOLDITALIC'}; 
	$fdfs{$fiparam{'BOLDITALIC'}} = 1;
    }

    $node = "\"".$fiparam{'NAME'}."\",".$fiparam{'SIZE'}.",".
	$fiparam{'STYLE'}.",###,$norm,$bold,$ital,$bital,".$fiparam{'GEO'};

    push @defs, $node;
}

# Build the font datas
foreach (sort keys %fdfs) {
    open FDATA,"script/cnvfont.1bpp.pl < font/$_.fdf |";
    print <FDATA>;
    close FDATA;
}

# Link up the fontstyle nodes and print them out
$link = 'NULL';
$fnode = 0;
foreach (sort @defs) {
    $fnode++;
    print "struct fontstyle_node fsn$fnode = {\n";
    s/###/$link/;
    print "$_ };\n";
    $link = "&fsn$fnode";
}

print "struct fontstyle_node *fontstyles = $link;\n/* The End */\n";

# The End #
