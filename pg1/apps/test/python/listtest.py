#!/usr/bin/env python

import PicoGUI

# keep track of "internal names" for the widgets, for the sake
# of our informational event handler
wnames = {}
def name(widget, name):
    wnames[widget] = name
    widget.text = name

# Application
app = PicoGUI.Application('List test')
wnames[app] = 'Application'

# Scrolling box
wBox = app.addWidget('Box')
wBox.addWidget('Scroll', 'before').bind = wBox

# Add some normal listitems (just text)
# The listitem widget is hilighted when the cursor is over them,
# and turned on when clicked. They are mutually exclusive.
wItem = wBox.addWidget('ListItem', 'inside')
name(wItem, 'Normal listitem #0')
for i in range(1, 5):
    wItem = wItem.addWidget('ListItem')
    name(wItem, 'Normal listitem #%d' % i)

# Normally you'd want to use listitems, but just to show the
# difference we'll throw in some menuitems...
for i in range(5):
    wItem = wItem.addWidget('MenuItem')
    name(wItem, 'Normal menuitem #%d' % i)

# Instead of using the PG_WP_TEXT property, create some
# other widgets inside the listitem. They will be hilighted
# with their PG_WP_HILIGHTED property when the mouse is over them.
for i in range(5):
    wItem = wItem.addWidget('ListItem')
    wnames[wItem] = 'Container listitem widget #%d' % i
    cb = wItem.addWidget('Checkbox', 'inside')
    name(cb, 'li checkbox #%d' % i)
    cb.side = 'left'
    cb.sizemode = 'percent'
    cb.size = 20
    wLabel = cb.addWidget('Label')
    name(wLabel, 'Container listitem #%d' % i)
    wLabel.side = 'left'

# For contrast, repeat the same example with menuitems
for i in range(5):
    wItem = wItem.addWidget('MenuItem')
    wnames[wItem] = 'Container menuitem widget #%d' % i
    cb = wItem.addWidget('Checkbox', 'inside')
    name(cb, 'mi checkbox #%d' % i)
    cb.side = 'left'
    cb.sizemode = 'percent'
    cb.size = 20
    wLabel = cb.addWidget('Label')
    name(wLabel, 'Container menuitem #%d' % i)
    wLabel.side = 'left'


# Event handler to print out our hilight/unhilight events
def evtHilight(ev, widget):
    wname = wnames.get(widget, widget)
    if ev.name != 'idle':
        print `ev`, wname
        # this is a test case for a bug, and should be removed
        # when this bug is fixed
        app.server.free(widget)
        app.server.update()
app.link(evtHilight)

# Run
app.run()
