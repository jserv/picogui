<?php require "lib.php"; beginpage("Screenshots"); 

box("Screenshot index (a.k.a. the PicoGUI Museum)");
$shots = GetDirArray($path = "/home/groups/pgui/news","scrshot");
echo "<table cellspacing=1 cellpadding=2 border=1>\n";

for ($i=Count($shots)-1;$i>=0;$i-=2) {
   echo "<tr><td align=left valign=top><font color=\"#000000\">\n";
   scrshot_thumb($path,$shots[$i]);
   echo "</font></td><td align=left valign=top><font color=\"#000000\">\n";
   if ($i)
     scrshot_thumb($path,$shots[$i-1]);
   echo "</font></tr>\n";
}
   
echo "</table>\n";
endbox();
endpage(); ?>
