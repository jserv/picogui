import PicoGUI

print "Imporing python module"

def thread():
    print "Starting python thread..."

    # Load our widget template, importing widgets from it
    app = PicoGUI.TemplateApp(open("data/hud.wt").read(),[
        'VelocitySlider'
        ])

    # Event handler for the velocity slider
    def velocityChange(ev, widget):
        world.velocity = (1000-widget.value) / 150.0
    app.VelocitySlider.size = 1000
    app.VelocitySlider.value = 1000
    app.link(velocityChange,app.VelocitySlider,'activate')

    # Set up an input filter to catch all events that
    # aren't absorbed by any picogui widgets.
    # This adds a filter that recieves all events and absorbs none,
    # placed immediately after the input filter that dispatches pointing
    # events to picogui widgets.
    def myfilter(t, sender):
        print (t.dev, t.name, t.sender, t.__dict__)
    app.link(myfilter, app.addInfilter(
        app.server.getresource('infilter pntr dispatch')))

    app.run()

    print "Python thread ending"
