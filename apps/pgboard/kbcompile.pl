#!/usr/bin/perl
# $Id: kbcompile.pl,v 1.6 2001/07/19 09:06:38 micahjd Exp $
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

$formatver = 1;

# Load a symbol table
foreach $file (map {glob($_)} 
	"/usr/local/include/picogui/*.h",
	"/usr/include/picogui/*.h") {

      open HFILE,$file;
      while (<HFILE>) {
	    if (/#define\s+(\S+)\s+(\S+)/) {
                # Yes, the eval here can be a bit insecure if the header
	        # files are compromised...
	        # Necessary to handle things like (1<<5) in the headers
		$sym = $1;
		$val = $2;
		$val = eval($val) if ($val =~ /<</);
		$symbols{$sym} = $val;
	    }
      }
      close HFILE;
}

$options{'side'} = $symbols{'PG_S_BOTTOM'};
$options{'size'} = 25;
$options{'sizemode'} = $symbols{'PG_SZMODE_PERCENT'};

# Actual processing
while (<>) {
      # Ignore blanks and comments
      s/#.*//;
      next if (!/\S/);

      # Switching sections?
      if (/^\[([^\]]+)/) {
      	 $pattern = $1;
	 push @pattern_list,$pattern;
	 next;
      }
      if (/^\:(\S+)/) {
      	 $section = $1;
	 next;
      }

      # Some braindead preprocessing
      foreach $key (keys %symbols) {
      	      s/$key/$symbols{$key}/ge;
      }
      s/(0x[0-9A-Fa-f]+)/eval($1)/ge;
      s/\'(.)\'/ord($1)/ge;

      # Miscellaneous options
      if ((!$pattern) && /^\s*(\S+)\s*=\s*(\S+)/) {
      	 $options{$1} = $2;
	 next;
      }

      # An entry in the :pattern section.
      if ($section eq 'pattern') {
         s/\s//g;
	 ($cmd,@param) = split(/,/,$_);
	 # Find commands that need request loaders
	 $pnum = 0;
	 foreach (@param) {
	   $pnum++;                 # This will be 1-based, accounts for pgcommand header
	   next if (!/[\(\)\"]/);
	   # This is a request of some sort. Find the binary offset to this position
	   $offset = length($pat_table{$pattern}) + 4 * $pnum;
	   # Schtick a keyboard request header on
	   $req_table{$pattern} .= pack "N", $offset;
	   $req_count{$pattern}++;
	   
	   # Format the request itself
	   
	   if (/^\"([^\"]*)\"/) {
	      # String
       	      $req_table{$pattern} .= pack("nnN",$symbols{'PGREQ_MKSTRING'},0,length($1)).$1;
	   }
	   else {
	      die "Unknown request in: '$_'";
 	   }
	 }

	 # Pack into a pgcommand structure
	 $pat_table{$pattern} .= pack "n2N*", $cmd, scalar(@param), @param;
	 next;
      }

      # An entry in the :keys section. Pack it into the
      # key table in the current pattern
      if ($section eq 'keys') {
         s/\s//g;
	 # Pack to a struct key_entry
	 $key_table{$pattern} .= pack "n4Nn4", split(/,/,$_);
	 $key_count{$pattern}++;
      	 next;
      }
}

# Assemble data blocks for each pattern
foreach $pattern (@pattern_list) {
      $pattern_data .= pack("Nnn",length($pat_table{$pattern}),
      			    $req_count{$pattern},$key_count{$pattern}).
			    $pat_table{$pattern}.$req_table{$pattern}.
		            $key_table{$pattern};
}

# Assemble the patterns, name, and all header fields after the checksum
$file_data = pack("n6",$formatver,scalar(@pattern_list),
	          $options{'side'},$options{'size'},$options{'sizemode'},0
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
### The End ###
