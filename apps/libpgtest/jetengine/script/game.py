import PicoGUI, math


def inputHandler(t, sender):
    # Move the ship and camera in reaction to the mouse position
    if t.dev == 'mouse' and t.name == 'move':
        camera.roll = (t.x - 320) * 0.02
        ship.roll = (t.x - 320) * 0.05
        ship.altitude = 17 + (t.y - 240) * -0.02
        camera.altitude = 21 + (t.y - 240) * -0.04
        
def velocityChange(ev, widget):
    # Completely nonscientific equations to both set the velocity
    # of the moving texture, and stretch out the texture when going
    # fast, to give a magnified sense of motion.
    scale = 150.0
    max_v = 1000/scale
    v = (1000-widget.value)/scale
    world.velocity = v
    world.vrepeat = 5+math.pow(max_v,2)-math.pow(v,2)


def thread():

    # Load our widget template, importing widgets from it
    app = PicoGUI.TemplateApp(open("data/hud.wt").read(),[
        'VelocitySlider'
        ])

    # Event handler for the velocity slider
    app.VelocitySlider.size = 1000
    app.VelocitySlider.value = 1000
    app.link(velocityChange,app.VelocitySlider,'activate')

    # Set up an input filter to catch all events that
    # aren't absorbed by any picogui widgets.
    # This adds a filter that recieves all events and absorbs none,
    # placed immediately after the input filter that dispatches pointing
    # events to picogui widgets.
    app.link(inputHandler, app.addInfilter(
        app.server.getresource('infilter pntr dispatch')))

    app.run()
