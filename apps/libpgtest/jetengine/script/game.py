import PicoGUI
import sys

print "Python path:"
print sys.path

print "Starting python thread..."

app = PicoGUI.Application("foo")
app.run()

print "Python thread ending"
