#!/usr/bin/perl
#
# Little theme indexer
#

# Get a picture of the default theme for comparison
system("pgserver -nc indexer.pgconf");
system("mv themeshot0.ppm defaulttheme.ppm");
sleep 1;  # wait for pgserver to close
system("rm -f web/*");

open HTML, ">web/index.php";

# HTML header
print HTML <<EOF;
<?php require "../lib.php"; beginpage("Theme Index"); 
box("PicoGUI Themes");
?>

This is a catalog of PicoGUI themes, listing some information about each
theme along with a sample picture. You can download the compiled theme here.
Theme source code is in CVS. The indexing system is still rather primitive, so
the sample pictures may not accurately reflect the theme.
<p>

EOF

foreach $themefile (@ARGV) {
	$themefile =~ /([^\/]*)$/;
	$shortname = $1;
	$filesize = -s $themefile;

	# Run a test application on the theme	 
	system("pgserver -nc indexer.pgconf -t $themefile");
        sleep 1;  # wait for pgserver to close
        open INFILE,"themename";
	$themename = <INFILE>;
	chomp $themename;
	close INFILE;
	print "---- $themename\n";
	
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

# End the file
print HTML <<EOF;
<?php
endbox();
endpage(); ?>
EOF

### The End ###
