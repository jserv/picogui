#!/usr/bin/perl
# $Id$
#
# This script converts a .kbs keyboard definition source to the .kb
# binary representation as defined in kbfile.h
#
# This whole thing's quite simple compared to themec. All input/output is
# via stdin and stdout. I'm sure it's quite easy to break, and has no error
# reporting, so stick close to the format as shown in the example .kbs files
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

$formatver = 4;

# Default path to PicoGUI
my $PG_PATH = "/usr/local";

# Parse arguments

sub usage
  {
    print "Usage: $0 [-pg <PicoGUI path>]\n";
    exit (1);
  }

while (my $arg = shift)
  {
    if ($arg eq "-pg" && ($arg = shift))
      {
	$PG_PATH = $arg;
	last;
      }
    else
      {
	usage ();
      }
  }


# Load a symbol table
foreach $file (map {glob($_)} 
	"${PG_PATH}/include/picogui/*.h") {

      open HFILE,$file;
      while (<HFILE>) {
	    if (/#define\s+(\S+)\s+(\S+)/) {
                # Yes, the eval here can be a bit insecure if the header
	        # files are compromised...
	        # Necessary to handle things like (1<<5) in the headers
		$sym = $1;
		$val = $2;
		$val = eval($val) if ($val =~ /<</ or $val =~ /^0x/);
		$symbols{$sym} = $val;
	    }
      }
      close HFILE;
}

$option{'side'} = $symbols{'PG_S_BOTTOM'};
$option{'size'} = 0;
$option{'sizemode'} = $symbols{'PG_SZMODE_NORMAL'};

# Actual processing
while (<>) {
    last if (/^end$/);
    # Symbol table substitution
    foreach $key (keys %symbols) {
	s/$key/$symbols{$key}/ge;
    }
    $file .= $_;
}
eval $file;
die $@ if ($@);

# Assemble data blocks for each pattern
foreach $pattern (@pattern_list) {
      $pattern_data .= pack("Nnn",length($pat_table{$pattern}),
      			    $req_count{$pattern},$key_count{$pattern}).
			    $pat_table{$pattern}.$req_table{$pattern}.
		            $key_table{$pattern};
}

# Assemble the patterns, name, and all header fields after the checksum
$file_data = pack("n6",$formatver,scalar(@pattern_list),
	          $option{'side'},$option{'size'},$option{'sizemode'},0
		  ).$pattern_data;

# Assemble the chunk before the checksum (magic and length)
$file_prefix = "PGkb".pack("N",length($file_data)+12);

# Calculate checksum
$_ = $file_prefix.$file_data;
for ($i=0;$i<length($_);$i++) {
   $checksum += ord(substr($_,$i,1));
}
$checksum = pack("N",$checksum);

print $file_prefix.$checksum.$file_data;

###################### Functions called in the keyboard description

sub newpattern {
    # for "normal" (key) patterns
    $pattern = ++$pattern_num;
    push @pattern_list,$pattern;
}

sub execpattern {
    my ($cmd) = @_;
    newpattern;
    $key_count{$pattern} = 0;
    $pat_table{$pattern} = $cmd;
    $req_count{$pattern} = 1; #PGKB_REQUEST_EXEC
}

sub canvas {
    my ($cmd, @param) = @_;
    $pat_table{$pattern} .= pack "n2N*", $cmd, scalar(@param), @param;
}

sub hotspot {
    $key_table{$pattern} .= pack "n4Nn4", @_;
    $key_count{$pattern}++;
}

sub key {
    my ($x,$y,$w,$h,$k,$pk,$mod) = @_;
    hotspot($x,$y,$w,$h,0,$k,$pk,$mod,0);
}

sub patlink {
    my ($x,$y,$w,$h,$pat) = @_;
    hotspot($x,$y,$w,$h,0,0,0,0,$pat);
}

sub request {
    my ($pnum,$req,$data) = @_;

    # Schtick a keyboard request header on
    $req_table{$pattern} .= pack "N", length($pat_table{$pattern}) + 4 * $pnum;
    $req_count{$pattern}++;
	 
    $req_table{$pattern} .= pack("NNnn",0,length($data),$symbols{'PGREQ_'.$req},0).$data;
}

sub loadfile {
    my ($fname) = @_;
    my $data;
    open INFILE,$option{'path'}.$fname or die;
    $data = join '', <INFILE>;
    close INFILE;
    return $data;
}

###################### Graphics primitives

sub pixel {
    my ($x,$y) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_PIXEL'},$x,$y,1,1);
}

sub line {
    my ($x1,$y1,$x2,$y2) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_LINE'},
	   $x1,$y1,$x2-$x1,$y2-$y1);
}

sub rect {
    my ($x,$y,$w,$h) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_RECT'},
	   $x,$y,$w,$h);
}

sub frame {
    my ($x,$y,$w,$h) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_FRAME'},
	   $x,$y,$w,$h);
}

sub bar {
    my ($x,$y,$h) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_BAR'},
	   $x,$y,1,$h);
}

sub slab {
    my ($x,$y,$w) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_SLAB'},
	   $x,$y,$w,1);
}

sub ellipse {
    my ($x,$y,$w,$h) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_ELLIPSE'},
	   $x,$y,$w,$h);
}

sub fellipse {
    my ($x,$y,$w,$h) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_FELLIPSE'},
	   $x,$y,$w,$h);
}

sub fpolygon {
    my ($x,$y,$w,$h) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_FPOLYGON'},
	   $x,$y,$w,$h);
}

sub text {
    my ($x,$y,$s) = @_;
    request(6,'MKSTRING',$s);
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_TEXT'},
	   $x,$y,1,1,0);
}

sub bitmap {
    my ($x,$y,$w,$h,$bmp) = @_;
    request(6,'MKBITMAP',$bmp);
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_BITMAP'},
	   $x,$y,$w,$h,0);
}

sub tilebitmap {
    my ($x,$y,$w,$h,$bmp) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_TILEBITMAP'},
	   $x,$y,$w,$h,$bmp);
}

sub gradient {
    my ($x,$y,$w,$h,$angle,$c1,$c2) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_GRADIENT'},
	   $x,$y,$w,$h,$angle,$c1,$c2);
}

sub setcolor {
    my ($c) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_SETCOLOR'},
	   $c);
}

sub setfont {
    my ($f) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_SETFONT'},
	   $f);
}

sub font {
    my ($name,$size,$flags) = @_;
    request(2,'MKFONT',pack("a40Nnn",$name,$flags,$size,0));
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_SETFONT'},0);
}


sub setlgop {
    my ($l) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_SETLGOP'},
	   $l);
}

sub setangle {
    my ($a) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_SETANGLE'},
	   $a);
}

sub setsrc {
    my ($x,$y,$w,$h) = @_;
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_SETSRC'},
	   $x,$y,$w,$h);
}

sub setflags {
    my ($f) = @_;
    canvas($symbols{'PGCANVAS_DEFAULTFLAGS'},$f);
}

sub setmapping {
    my ($x,$y,$w,$h,$type) = @_;
    setflags($symbols{'PG_GROPF_UNIVERSAL'});
    canvas($symbols{'PGCANVAS_GROP'},$symbols{'PG_GROP_SETMAPPING'},
	   $x,$y,$w,$h,$type);
    canvas($symbols{'PGCANVAS_INPUTMAPPING'},
	   $x,$y,$w,$h,$type);
    setflags(0);
}

sub clear {
    canvas($symbols{'PGCANVAS_NUKE'});
}

### The End ###



