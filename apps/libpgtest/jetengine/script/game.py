import PicoGUI

print "Imporing python module"

def thread():
    print "Starting python thread..."

    print "foo is a " + repr(foo)
    foo.bleh = 42
    
    app = PicoGUI.TemplateApp(open("data/hud.wt").read())
    app.run()
    
    print "Python thread ending"
