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

# Menu from string
pgMenuFromString("This|is|a\nTest");
# Menu from array of handles
pgEnterContext();
pgMenuFromArray(pgNewString("Submenus:\n"),
                pgNewString("Programs"),
	        pgNewString("Secrets"),
	        pgNewString("Junk"),
                pgNewString("Options"));
pgMenuFromArray(pgNewString("A"),
                pgNewString("B"),
	        pgNewString("C"));
pgLeaveContext();

##### Popup box to test pgGetEvent

pgEnterContext();
$popup = pgNewPopup(160,100);
pgNewWidget(PG_WIDGET_LABEL); 
pgSetWidget(PGDEFAULT,
	    PG_WP_TEXT, pgNewString("pgGetEvent() Test"),
	    PG_WP_TRANSPARENT,0,
	    PG_WP_STATE, PGTH_O_LABEL_DLGTITLE);

$toolbar = pgNewWidget(PG_WIDGET_TOOLBAR);
pgSetWidget($toolbar,PG_WP_SIDE,PG_S_BOTTOM);
pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,$toolbar);
pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Thwamp!"),PG_WP_SIDE,PG_S_LEFT);
pgNewWidget(PG_WIDGET_BUTTON);
pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Quack!"),PG_WP_SIDE,PG_S_RIGHT);

$wLabel = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_AFTER,$toolbar);
# Canvas with some junk in it
$c = pgNewWidget(PG_WIDGET_CANVAS);
pgWriteCmd($c,PGCANVAS_GROP,PG_GROP_FRAME, 80,25,5,5);
pgWriteCmd($c,PGCANVAS_SETGROP,1,0);
pgWriteCmd($c,PGCANVAS_COLORCONV,1,1);
pgWriteCmd($c,PGCANVAS_GROP,PG_GROP_LINE, 10,10,50,20);
pgWriteCmd($c,PGCANVAS_SETGROP,1,0);
pgWriteCmd($c,PGCANVAS_COLORCONV,1,1);
pgWriteCmd($c,PGCANVAS_GROP,PG_GROP_LINE, 30,10,-12,30);
pgWriteCmd($c,PGCANVAS_SETGROP,1,0);
pgWriteCmd($c,PGCANVAS_COLORCONV,1,1);

# Little event loop
while ($evt{from} != $popup) {
      %evt = pgGetEvent();
      pgReplaceText($wLabel,join(',',%evt));
      
      # Move the little frame (grop #0) when canvas is clicked
      if ($evt{from} == $c) {
      	 pgWriteCmd($c,PGCANVAS_FINDGROP,0);
	 pgWriteCmd($c,PGCANVAS_MOVEGROP,$evt{x}-3,$evt{y}-3,5,5);
	 pgWriteCmd($c,PGCANVAS_REDRAW);
      }
}
pgLeaveContext();

### Make an application with a 'real' event loop
### As a bonus, it uses anonymous subroutines to show pgBind the way
### it was meant to work! (reminds ya of Perl/Tk, doesn't it ;-)

pgRegisterApp(PG_APP_NORMAL,"Perl PicoGUI test app");
$toolbar = pgNewWidget(PG_WIDGET_TOOLBAR);
$wLabel1 = pgNewWidget(PG_WIDGET_LABEL);
$wLabel2 = pgNewWidget(PG_WIDGET_LABEL);
pgSetWidget(PGDEFAULT,PG_WP_ALIGN,PG_A_SW,PG_WP_SIDE,PG_S_ALL);

# Some buttons with their own handlers
pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,$toolbar);
pgReplaceText(PGDEFAULT,"Start");
pgBind(PGDEFAULT,PGBIND_ANY,sub {
    pgReplaceText($wLabel1,"Clickski!");
    return 0;
});

pgNewWidget(PG_WIDGET_BUTTON);
pgReplaceText(PGDEFAULT,"Stop");
pgBind(PGDEFAULT,PGBIND_ANY,sub {
    pgReplaceText($wLabel1,"Exski!");
    return 0;
});

pgNewWidget(PG_WIDGET_BUTTON);
pgReplaceText(PGDEFAULT,"Procrastinate");
pgBind(PGDEFAULT,PGBIND_ANY,sub {
    pgReplaceText($wLabel1,"Checkski!");
    return 0;
});

# plus a generic handler, using the pgEvent it was passed  
pgBind(PGBIND_ANY,PGBIND_ANY,sub {
    my %evt = @_;
    my $text = "";
    
    foreach (sort keys %evt) {
        $text .= "$_ = $evt{$_}\n";
    }
    $text .= "\nWidget text: ".pgGetString(pgGetWidget($evt{from},PG_WP_TEXT));
    
    pgReplaceText($wLabel2,$text);
    return 0;
});

pgEventLoop();


### The End ###
