#!/usr/bin/perl
#
# Little theme indexer
#

# Get a picture of the default theme for comparison
system("pgserver -nc indexer.pgconf >/dev/null 2>/dev/null");
system("mv themeshot1.ppm defaulttheme.ppm");

system("rm -f web/*");

open HTML, ">web/index.html";

foreach $themefile (@ARGV) {
	$themefile =~ /([^\/]*)$/;
	$shortname = $1;
	$filesize = -s $themefile;

	# Run a test application on the theme	 
	system("pgserver -nc indexer.pgconf -t $themefile >/dev/null 2>/dev/null");
	open INFILE,"themename";
	$themename = <INFILE>;
	chomp $themename;
	close INFILE;
	print "$themename\n";
	
	# Now we have a screenshot of the bare background and cursor in
	# themeshot0.ppm and a screenshot of the app in themeshot1.ppm
	# If the app looks no different than default, we should show the background.
	if (!system("cmp -s defaulttheme.ppm themeshot1.ppm")) {
		`mv themeshot0.ppm themeshot1.ppm`;
	}
  	`convert themeshot1.ppm web/$shortname.png`;
	`cp $themefile web/$shortname`;

	# Print some HTML
	print HTML <<EOF;
<table border=1><tr><td>
  <img src="$shortname.png" width=96 height=96>
</td><td valign=top>
  <font size="+1"><b>$themename</b></font><p>
  File: $shortname <br>
  Size: $filesize bytes<p>
  <a href="$shortname">[Download Binary]</a>
</td></tr></table><br>

EOF

}
