import PicoGUI, math, time

print "Python module imported"

class input:
    def handler(self, t, sender):
        # Move the ship and camera in reaction to the mouse position
        if t.dev == 'mouse':
            if t.name == 'down':
                self.lastX = t.x
                self.lastY = t.y
            if t.name == 'move' and t.buttons:
                dx = t.x - self.lastX
                dy = t.y - self.lastY
                self.lastX = t.x
                self.lastY = t.y
                camera.yaw -= dx * 0.1
                camera.pitch += dy * 0.1
        
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
    print "Starting game thread"

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
    app.link(input().handler, app.addInfilter(
        app.server.getresource('infilter pntr dispatch')))

    # FIXME: This time.sleep seems to keep the two threads from
    #        fighting over the python lock a bunch and being slow
    #        for the first several seconds of execution.
    print "Unnecessary delay"
    time.sleep(1)

    print "Running"
    app.run();

