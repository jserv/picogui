import PicoGUI, time

print "Imporing python module"

def thread():
    print "Starting python thread..."

    # Load our widget template, importing widgets from it
    app = PicoGUI.TemplateApp(open("data/hud.wt").read(),[
        'VelocitySlider'
        ])

    # Event handler for the velocity slider
    def velocityChange(ev, widget):
        world.velocity = widget.value / 10000.0
    app.link(velocityChange,app.VelocitySlider,'activate')
    
    app.run()

    print "Python thread ending"
