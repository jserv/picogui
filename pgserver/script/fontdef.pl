#!/usr/bin/perl
#
# This reads in .fi files, and creates the static linked list
# of font styles.  It also uses cnvfont to load the .fdf files
#

print "#include <font.h>\n\n";

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
    $fiparam{'STYLE'} = join('|',map('FSTYLE_'.$_,
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
