import sys, time, PicoGUI

print "Python path:"
print sys.path

print "Starting python thread..."

app = PicoGUI.Application("Foo");
app.run()

print "Python thread ending"
