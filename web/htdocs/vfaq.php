<?php require "lib.php"; beginpage("Virtual FAQ"); 
box("Virtual FAQ");
echo "<div align=right>Last updated: <b>October 29, 2000</b> (micah)</div><p>\n";
function question($txt) {
  echo "<p><center><b>$txt</b></center><hr>\n";
}
?>

<?php question("Why is this called a Virtual FAQ?"); ?>
Well, to my knowledge nobody has asked me these exact questions, so I can
hardly call them "Frequently Asked". But, they seem useful anyway and if a
hypothetical person were to ask them they would most likely look for the
answer under a "FAQ" section, so I might as well save them the trouble.

<?php question("So what is this PicoGUI thing anyway?"); ?>
In short, it is a GUI. It's the layer between an application and the screen.
It shares many elements of other GUIs, but it's designed with handheld
computers or low-powered desktops in mind. The following outlines the major
differences between PicoGUI and other GUIs I have seen:<ul>
<li><b>Architecture</b><br>
  Like the <a href="http://xfree86.org">
  X Window System</a>, it has a flexible client-server architecture. But,
  unlike X, fonts, bitmaps, widgets, and anything else the application would
  need are built in to the server. This sacrifices a small decrease in
  flexibility for an increase in speed and a large decrease in size. Because
  of things like its standard client interface, powerful themes, and modular
  drivers it can be more flexible than other other small GUIs.
</li>
<li><b>Size</b><br>
  Stripped ELF binaries of PicoGUI are usually around <b>100K</b>. This includes
  the server, fonts, and all other vital data. Many things can be enabled or
disabled at compile-time, affecting the size. Themes' size are mainly
  dependant on any bitmaps they contain. They usually range from about 300
  bytes to hundreds of kilobytes. A typical theme like the Aqua theme
  featured some of the screenshots is about 20K. This is expected to decrease
  when compressed bitmaps are implemented. The client library (used by
  PicoGUI applications) is about 1000
  lines of code, and compiles to about <b>6K</b>.
</li>
<li><b>Memory footprint</b><br>
  Like PicoGUI's CPU usage, much of its memory usage is dependant on the
  video driver in use. The memory usage of the PicoGUI server itself is easy
  to measure, however. These values were all measured on October 29:
  Immediately after starting it up, with no theme loaded, it uses <b>5K</b> of
  memory. (Much of this is the buffer for the mouse cursor) With no theme, and
  a demo application (see the October 25 screenshot) it uses almost <b>13K</b> of
  memory. With the demo app and the Aqua theme loaded, it uses <b>42K</b> of memory.
  Much of this memory is allocated in very small (12 to 50 byte) chunks,
  so actual mileage may vary until I implement a heap for the small reusable
  structures. As always, more optimization is planned.
</li>
<li><b>Screen real-estate</b><br>
  PicoGUI's themes are more than an afterthought to make the screen pretty,
they are an integral part of PicoGUI that can be used to customize the size,
shape, and appearance of objects to the application or screen geometry.
PicoGUI has <b>no overlapping windows</b>, instead each application occupies
a resizable "panel" that sticks to the edge of the screen and to other
applications. This part of PicoGUI isn't finished yet, but hopefully it will
conserve screen space while still allowing full multitasking with ease. For
dialog boxes, messages, and menus, PicoGUI supports layered modal popup
boxes.
</li>
</ul>  

<?php question("Why would you make such a crazy thing?"); ?>
<ul><li>
<b>Short answer:</b> None of the other small GUIs I've seen fit my needs- they all
seem like a smaller clone of a desktop GUI.
</li><li>
<b>Longer answer:</b> PicoGUI started life as "guwi", a GUI for the <a
href="http://kiwi.sourceforge.net">Kiwi Project</a>. For information on this
odd first couple weeks of PicoGUI, see the oldest screenshots in the <a
href="scrshots.php">PicoGUI Museum</a>, and the <a
href="http://kiwi.sourceforge.net/attic.html">Kiwi Project's Attic.</a>
</li></ul>

<?php question("Where did the name come from?"); ?>
Well, besides the fact that it sounds kinda cute, I felt like poking a bit
of fun at the <a href="http://microwindows.org">tradition</a> of naming <a
href="http://www.uclinux.org">embedded systems projects</a> using metric prefixes.
<br>;-)

<?php question("Why is it so darn small?"); ?>
I don't know... I was about to ask why most GUIs are so darn big. Mainly
because they have so many extra features, I suppose. I'm trying to keep the
feature set of PicoGUI down to just what's needed, but if
not-totally-necessary-but-cool things like scalable fonts need implementing,
they will be optional at compile time. A possibly even larger influence,
though, is the mindset of the programmer. When given large hardware, far too
many programmers will make large programs. This is the core of Gates' Law. I
try to imagine a small system even when I'm on my Celery 366. <br>
<i>Only you can prevent creeping featurism!</i>

<?php question("What are the minimum system requirements?"); ?>
This heavily depends on the video driver, where most of the time is spent. I
have just recently got the SVGAlib driver working, so I haven't had time yet
to work with it and optimize it on lower-end systems, but the goal is for it to be usable
even on a machine the speed of a Palm Pilot.

<?php question("What screen resolutions does it support?"); ?>
Theoretically, anything from 1x1 to about 4096x4096. If you mean what
resolutions will it work well with, that depends on the theme. By modifying
spacing values and margins, PicoGUI can go from quite a scrunched-up look
(for those little 128x64 screens) to quite roomy. See the <a
href="scrshots.php">screenshot gallery</a> for examples- most of the
screenshots are at 640x480 or 320x240, but some of the old ones are at
240x64. (the resolution of my <a href="http://kiwi.sourceforge.net">Kiwi</a>
prototype's screen)

<?php question("Does PicoGUI have themes?"); ?>
Yes! PicoGUI's themes have a simple format, but can customize almost any
value. Procedures for drawing things are described in a simple stack-based
interpreted programming language. PicoGUI themes are originally described in
a language with a C-like syntax, and converted to a packed and sorted binary
theme using the <i>themec</i> utility from the themetools package.
Themes have a hierarchial structure, and multiple themes can be loaded into
the server at once. For example, a general 'look-and-feel' theme, a theme to
customize the fonts, and a theme to internationalize dialog box buttons
could all be loaded at the same time.

<?php question("What operating system does PicoGUI work with?"); ?>
Currently all development and testing is being performed on Linux, but
PicoGUI should <i>in theory</i> run on any network operating system with very
little change.
At one point, PicoGUI would compile and run on Windows, but some of the
added command-line parsing code broke this. (If anybody is interested in a
Windows, port, just ask and I'll try to fix it)
We are also working on porting PicoGUI to embedded hardware.

<?php question("What video hardware does PicoGUI work with?"); ?>
PicoGUI's video system is very modular, so new drivers are easy to develop.
Currently it has drivers for <a href="http://www.libsdl.org">SDL</a> and <a
href="http://www.svgalib.com">SVGAlib</a>. It will run with or without X.

<?php question("How do the clients communicate with the PicoGUI server?"); ?>
Shared memory support is planned, but currently all communication is via
TCP/IP. Consequently, PicoGUI is network-transparent. No additional layers
such as a widget toolkit or window manager are required.

<?php question("So what's so different about PicoGUI's architecture?"); ?>
I'd say the most different thing is the lack of overlapping windows. This is
not an omission, but a conscious design decision to both simplify the
graphics engine, simplify the user's experience, and conserve screen space
while still supporting multitasking. <br>
The next thing that sets it apart from the other small GUIs I've seen, is
that I'm trying to make something featureful enough that it can be a full
desktop environment. The other GUIs are generally something designed for a
single specialized application, whereas I'm trying to make something with
consistant protocols and the ability to run a wide range of applications,
yet still conserve size, memory, CPU, and screen space.<br>
It's a big goal, I hope I can meet it.<br>:)

<?php
endbox();
endpage(); ?>
