import PicoGUI, time

print "Imporing python module"

def thread():
    print "Starting python thread..."

    app = PicoGUI.TemplateApp(open("data/hud.wt").read())
    app.server.update()

    while 1:
        world.x += 0.1;
        time.sleep(0.1);
    
    print "Python thread ending"
