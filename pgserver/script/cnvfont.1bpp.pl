#!/usr/bin/perl
#
# This program generates a file with the fdf font data converted
# to 1bpp bitmap data, formatted for input by a C compiler.
#
# Micah Dowty <micah@homesoftware.com>
#

$hspace = 1;
$vspace = 1;
$fg = 15;
$bg = 0;
$num = $size = 0;

for ($i=0;$i<256;$i++) {
    push @trtab,-1;
    push @vwtab,0;
}

while (<>) {
    chomp;
    if (/^\s*\#/) {
	print "/* $_ */\n";
    }
    elsif (/\[([^\]]*)\](.*)/) {
	$fntname = $1;
	$param = $2;
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
	while (<>) {
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
print "\nlong ${fntname}_tr[256] = {\n";
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
    print "\nunsigned char ${fntname}_vw[256] = {\n";
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
print "\nunsigned char ${fntname}_bits[$size] = {\n";
$com = ',';
for ($i=0;$i<$num;$i++) {
    $com = ' ' if ($i==$num-1);
    print "\t$bitmaps[$i]$com\t/* $i */\n";
}
print "};\n";

print "\nstruct font $fntname = {\n\t";
print "${fntname}_bits, $h, $hspace, $vspace, ${fntname}_vw, ${fntname}_tr";
$hdrs = 256*5;
print "\n};\n";

$totalsize = $hdrs+$size;
print STDERR "Summary of font [$fntname] :\n";
print STDERR "\theaders = $hdrs\n";
print STDERR "\tchardata = $size\n";
print STDERR "\tnumchars = $num\n";
print STDERR "\ttotalsize = $totalsize\n";

# The End







