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
        world.velocity = (1000-widget.value) / 100000.0
    app.VelocitySlider.size = 1000
    app.VelocitySlider.value = 1000
    app.link(velocityChange,app.VelocitySlider,'activate')
    
    app.run()

    print "Python thread ending"
