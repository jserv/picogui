<?php

################################# News

function GetDirArray($sPath,$exten) { 
   //Load Directory Into Array 
   $handle=opendir($sPath); 
   while ($file = readdir($handle)) {
     if (ereg("\\.".$exten."$",$file) && !ereg("latest",$file)) {
	 $retVal[count($retVal)] = $file;
     }
   }
   //Clean up and sort 
   closedir($handle); 
   sort($retVal); 
   return $retVal; 
}

# Convert the fields in a filename to an HTML title thingy
function name2html($filename) {
      $months = array("January","February","March","April","May","June","July",
		      "August","September","October",
		      "November","December");
   
      $iteminfo = explode(".",$filename);
      $author = $iteminfo[2];
      $year = substr($iteminfo[0],0,4);
      $month = $months[substr($iteminfo[0],4,2)-1];
      $day = substr($iteminfo[0],6,2);
      return "<b>$month $day, $year</b> ($author)";
}

# Load the news items, put 'em into a box
function newsbox($title,$path) {
   box($title);
   $newses = GetDirArray($path,"news");
   for ($i=count($newses)-1;$i>=0;$i--) {
      $f = fopen($path."/".$newses[$i],"r");
      $item = fread($f,filesize($path."/".$newses[$i]));
      fclose($f);
      $title = name2html($newses[$i]);
      echo "<p><div align=right>$title</div><hr>\n".$item;
   }
   endbox();
}

################################# Page headers
function beginpage($name) {

echo "<html><head><title>PicoGUI - (${name})</title></head>\n";
?>
<body bgcolor="#000000" text="#FFFFFF">
<table cellpadding=5 cellspacing=5><tr>
<td valign=top width=230>
<a href="logoinfo.php">
<img src="/gfx/logo-micah2.png" width=230 height=150
alt="PicoGUI Logo" border=0>
</a><p>
<?php sidebar(); ?>
</td><td valign=top>

<table width="100%"><tr>
   <td align=left valign=center>
     <table><tr><td align=center>
     <a href="http://sourceforge.net/projects/pgui/">
     <img src="/gfx/sf.png" border=0 width=45 height=45></a>
     </td></tr><tr><td align=center>
     <font size="-1">SF Summary<br>Page</font></td></tr></table>
     
   </td><td align=left valign=center>
     <table><tr><td align=center>
     <a href="news.php">
     <img src="/gfx/news.png" border=0 width=45 height=45></a>
     </td></tr><tr><td align=center>
     <font size="-1">News</font></td></tr></table>
     
   </td><td align=left valign=center>
     <table><tr><td align=center>
     <a href="scrshots.php">
     <img src="/gfx/scrshot.png" border=0 width=45 height=45></a>
     </td></tr><tr><td align=center>
     <font size="-1">Screenshots</font></td></tr></table>

   </td><td align=left valign=center>
     <table><tr><td align=center>
     <a href="vfaq.php">
     <img src="/gfx/vfaq.png" border=0 width=45 height=45></a>
     </td></tr><tr><td align=center>
     <font size="-1">Virtual<br>FAQ</font></td></tr></table>

   </td><td align=left valign=center>
     
     <table><tr><td align=center>
     <font size="-1">
     Hosted by: <br>
     <A href="http://sourceforge.net">
     <img src="http://sourceforge.net/sflogo.php?group_id=4764&type=1"
     width=88 align=absmiddle
     height=31 border=0 alt="SourceForge"></A>
     <br>It's nifty!
     </font>
     </td></tr></table>
  
</td></tr></table><br>
  
 <?php
}

################################# Sidebar
function sidebar() {

   box("Download source code");
   linkitem("http://pgui.sourceforge.net/cvstgz/pgui-dev-latest.tar.gz",
	    "Latest nightly snapshot");
   linkitem("snapshotlist.php",
	    "Snapshot archive");
   linkitem("http://cvs.sourceforge.net/cgi-bin/cvsweb.cgi/?cvsroot=pgui",
	    "Browse CVS");
   linkitem("http://sourceforge.net/cvs/?group_id=4764","CVS info");
   endbox();

   # Look up the most recent screenshot
   box("Latest screenshot");
   $shots = GetDirArray($path = "/home/groups/pgui/news","scrshot");
   $latest = $shots[Count($shots)-1];
   scrshot_thumb($path,$latest);
   echo "<p><center>(<a href=\"scrshots.php\">more screenshots</a>)</center>\n";
   endbox();

   box("Contacts");
   linkitem("mailto:micahjd@users.sourceforge.net","Micah Dowty");
   linkitem("http://lists.sourceforge.net/mailman/listinfo/pgui-devel",
            "Development mailing list");
   endbox();
}

################################# Screnshots

# Display a screenshot in thumbnail
function scrshot_thumb($path,$file) {
   $f = fopen($path."/".$file,"r");
   $img = chop(fgets($f,256));
   $title = name2html($file);
   $desc = fread($f,10000);
   echo "<center>$title<br><br><a href=\"/scrshots/$img.png\"><img \n";
   echo "  src=\"/scrshots/thumb.$img.png\" border=0 width=164 height=124></a><br><br>\n";
   echo "</center>$desc\n";
}   

################################# Ending
function endpage() {
  echo "</td></tr></table></body></html>\n";
}

################################# Link lists (in boxes)

function linkitem($url,$text) {
  echo "- <a href=\"$url\">$text</a><br>\n";
}

################################# Box
function box($title) {
?>
<table width="100%" cellpadding=2 cellspacing=0 border=0>
<tr bgcolor="#556677"><td>
 <table width="100%" cellpadding=5 cellspacing=0 border=0>
 <tr bgcolor="#000080"><td><font color="#FFFFFF">
<?php echo $title; ?>
 </font></td></tr>
 <tr bgcolor="#BBCCDD"><td><font color="#000000">
<?php
}
function endbox() {
   echo "</font></td><tr></table></td><tr></table><p>\n";
}
# The End
?>
