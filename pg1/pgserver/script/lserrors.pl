#!/usr/bin/perl
#
# Print a sorted list of the error messages in use
#
# - Micah Dowty <micahjd@users.sourceforge.net>
#

# Search all source, marking errors that have been used
open FINDF, "find -name '*.c' -o -name '*.h' |";
while (<FINDF>) {
   chomp;
	$fname = $_;
	$line = 0;
	open IFILE,$fname;
	while (<IFILE>) {
	   $line++;
      next if (/\#define/);
		if (/mkerror\([^,]*,\s*([0-9]+)\)/) {
         $errs_used{$1} .= "\n\t$fname:$line";
		}
	}
   close IFILE;
}
close FINDF;

# Record errors from the static table
open TABF, "gcore/defaulttext.inc";
while (<TABF>) {
   next if (!/\/\*\s*([0-9]+)/);
	$line = $1;
	next if (!/\"([^\"]*)/);
	$err_tab{$line} = $1;
}
close TABF;

foreach ( sort {$a<=>$b} (keys %errs_used, keys %err_tab) ) {
   next if ($_ == $prev);
	$prev = $_;
	
	if ($errs_used{$_} && $err_tab{$_}) {
   	next;
	}
	elsif ($errs_used{$_}) {
	   printf "%3d: DOES NOT EXIST, used in: %s\n", $_, $errs_used{$_};	
	   $output = 1;	
   }
	else {
		printf "%3d: UNUSED \"%s\"\n", $_, $err_tab{$_};
		$output = 1;
   }
}
if (!$output) {
   printf "No errors found in error table\n";
}

### The End ###
