import PicoGUI

print "Starting python thread..."

#print "About to run foo.bleh = 42"
#foo.bleh = 42
#print "Ran foo.bleh = 42"

app = PicoGUI.TemplateApp(open("data/hud.wt").read())
app.run()

print "Python thread ending"
