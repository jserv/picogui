# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'

######################### We start with some black magic to print on failure.

# Change 1..1 below to 1..last_test_to_print .
# (It may become useful if the test is moved to ./t subdirectory.)

BEGIN { $| = 1; print "1..1\n"; }
END {print "not ok 1\n" unless $loaded;}
use PicoGUI;
$loaded = 1;
print "ok 1\n";

######################### End of black magic.

# Insert your test code below (better if it prints "ok 13"
# (correspondingly "not ok 13") depending on the success of chunk 13
# of the test code):

pgMessageDialog("PicoGUI test","Hello!\nThis is a simple dialog");

print "Dialog returned: ".
      pgMessageDialog("Perl Module","This is a test!\nHello, world",
			       PG_MSGBTN_OK | PG_MSGBTN_CANCEL)."\n";
			       
# Make a custom dialog
pgEnterContext();
pgNewPopup(160,80);
$toolbar = pgNewWidget(PG_WIDGET_TOOLBAR);
pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,$toolbar);
pgReplaceText(PGDEFAULT,"Thwamp!");
pgNewWidget(PG_WIDGET_BUTTON);
pgReplaceText(PGDEFAULT,"Quack!");
$wLabel = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_AFTER,$toolbar);
pgNewWidget(PG_WIDGET_CANVAS);
pgUpdate();
# Little event loop
while (1) {
      pgReplaceText($wLabel,join ',',pgGetEvent());
}
pgLeaveContext();

# Make an application
pgRegisterApp(PG_APP_NORMAL,"Perl PicoGUI test app");
pgUpdate();
<STDIN>;
