import PicoGUI

print "Starting python thread..."

app = PicoGUI.TemplateApp(open("data/hud.wt").read())
app.server.mktheme(open("data/holographic.th","rb").read());

app.run()

print "Python thread ending"
