import PicoGUI, math


class input:
    lastClick = (0,0,0,0)

    def handler(self, t, sender):
        # Move the ship and camera in reaction to the mouse position
        if t.dev == 'mouse':
            if t.name == 'down':
                self.lastClick = (t.x, t.y, camera.yaw, camera.pitch)
            if t.name == 'move' and t.buttons:
                camera.yaw = float(t.x - self.lastClick[0] + self.lastClick[2])
                camera.pitch = float(t.y - self.lastClick[1] + self.lastClick[3])
        
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
    app.link(input().handler, app.addInfilter(
        app.server.getresource('infilter pntr dispatch')))

    app.run()
