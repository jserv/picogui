#!/usr/bin/perl
#
# Little theme indexer
#

# Get a picture of the default theme for comparison
system("pgserver -nc indexer.pgconf >/dev/null 2>/dev/null");
system("mv themeshot1.ppm defaulttheme.ppm");

foreach $themefile (@ARGV) {
	$themefile =~ /([^\/]*)$/;
	$shortname = $1;

	# Run a test application on the theme	 
   print "Processing theme $themefile\n";
   system("pgserver -nc indexer.pgconf -t $themefile >/dev/null 2>/dev/null");
	open INFILE,"themename";
	$themename = <INFILE>;
   close INFILE;
	print "  Name: $themename\n";
	
	# Now we have a screenshot of the bare background and cursor in
	# themeshot0.ppm and a screenshot of the app in themeshot1.ppm
	# If the app looks no different than default, we should show the background.
	if (!system("cmp -s defaulttheme.ppm themeshot1.ppm")) {
		`mv themeshot0.ppm themeshot1.ppm`;
	}
  	system("mv themeshot1.ppm web/$shortname.ppm");
}
