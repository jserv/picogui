<?php require "lib.php"; beginpage("CVS Snapshots"); 
box("CVS Snapshot Archive");
print "This archive covers the last 30 days. For anything older, use the CVS.<p>\n";

$months = array("January","February","March","April","May","June","July",
		"August","September","October",
		"November","December");

$h = opendir($path = "/home/groups/pgui/htdocs/cvstgz");
while ($file = readdir($h)) {
   if (ereg("^pgui-dev",$file) && !ereg("latest",$file)) {
      $stats = stat($path."/".$file);
      $year = substr($file,8,4);
      $month = $months[substr($file,12,2)-1];
      $day = substr($file,14,2);
      print "<a href=\"/cvstgz/$file\"><b>$month $day, $year</b></a> ($stats[7] bytes)<BR>\n";
   }
}
closedir($h);

endbox();
endpage(); ?>
