#!/usr/bin/perl
# $Id: fontdef.pl,v 1.11 2001/04/25 09:54:32 gobry Exp $
#
# This reads in .fi files, and creates the static linked list
# of font styles.  It also uses cnvfont to load the .fdf files
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
    open FDATA,"$fontdir/$_.fdf";

    $hspace = 1;
    $vspace = 1;
    $fg = 15;
    $bg = 0;
    $num = $size = 0;

    @trtab = ();
    @vwtab = ();
    @bitmaps = ();

    for ($i=0;$i<256;$i++) {
	push @trtab,-1;
	push @vwtab,0;
    }
    
    while (<FDATA>) {
	chomp;
	if (/^\s*\#/) {
	    print "/* $_ */\n";
	}
	elsif (/\[([^\]]*)\](.*)/) {
	    $fntname = $1;
	    $param   = $2;
	    if ($param =~ /\(\s*([\-0-9]+)\s*,\s*([\-0-9]+)\s*\)/) {
		$hspace = $1;
		$vspace = $2;
	    }
	    if ($param =~ /b([\-0-9]+)/) {
		for ($i=0;$i<256;$i++) {
		    $vwtab[$i] = $1;
		}
	    }
	}
	elsif (/:/) {
	    if (/\'(.)\'/) {
		$asc = ord($1);
	    }
	    elsif (/\#([0-9]+)/) {
		$asc = $1;
	    }
	    $trtab[$asc] = $size;
	    $dat = '';
	    $h = 0;
	    while (<FDATA>) {
		last if (!/\S/);
		s/\s//g;
		$w = length $_;
		s/\./0/g;
		s/[^0]/1/g;
		while ((length($_)%8)) {$_ .= '0'}
		$dat .= $_;
		$h++;
	    }
	    $_ = $dat;
	    $dat = '';
	    $bw = 0;
	    while (s/^(.{8})//) {
		$dat .= '0x'.unpack('H2',pack('B8',$1)).',';
		$bw++;  # bytes
	    }
	    chop $dat;
	    $vwtab[$asc] = $w;
	    $size += $bw;
	    push @bitmaps,$dat;
	    $num++;
	}
    }
    
    # Character translation table
    print "\nlong const ${fntname}_tr[256] = {\n";
    $com = ',';
    for ($i=0;$i<256;$i++) {
	$com = ' ' if ($i==255);
	if ($i>=ord(' ') and $i<=ord('z')) {
	    $c = '\''.chr($i)."' ";
	}
	else {
	    $c = '';
	}
	print "\t$trtab[$i]$com\t/* $i ${c}*/\n";
    }
    print "};\n";
    
    # Variable width table
    if (!$fixed) {
	print "\nunsigned char const ${fntname}_vw[256] = {\n";
	$com = ',';
	for ($i=0;$i<256;$i++) {
	    $com = ' ' if ($i==255);
	    if ($i>=ord(' ') and $i<=ord('z')) {
		$c = '\''.chr($i)."' ";
	    }
	    else {
		$c = '';
	    }
	    print "\t$vwtab[$i]$com\t/* $i $c*/\n";
	}
	print "};\n";
    }
    
    # Bitmap data
    print "\nunsigned char const ${fntname}_bits[$size] = {\n";
    $com = ',';
    for ($i=0;$i<$num;$i++) {
	$com = ' ' if ($i==$num-1);
	print "\t$bitmaps[$i]$com\t/* $i */\n";
    }
    print "};\n";
    
    print "\nstruct font const $fntname = {\n\t";
    print "${fntname}_bits, $h, $hspace, $vspace, ${fntname}_vw, ${fntname}_tr";
    $hdrs = 256*5;
    print "\n};\n";
    
    $totalsize = $hdrs+$size;
    print STDERR "Summary of font [$fntname] :\n";
    print STDERR "\theaders = $hdrs\n";
    print STDERR "\tchardata = $size\n";
    print STDERR "\tnumchars = $num\n";
    print STDERR "\ttotalsize = $totalsize\n";

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
    $link = "&fsn$fnode";
}

print "struct fontstyle_node *fontstyles = $link;\n/* The End */\n";

# The End #
