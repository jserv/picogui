import PicoGUI, math, time
import hud, input, mainmenu

print "Python module imported"

def thread():
    print "Starting game thread"

    # FIXME: This time.sleep seems to keep the two threads from
    #        fighting over the python lock a bunch and being slow
    #        for the first several seconds of execution.
    print "Unnecessary delay"
    time.sleep(1)
    game()


class game:
    def __init__(self):
        self.app = PicoGUI.InvisibleApp()

        self.camera = camera
        self.world = world
        self.skybox = skybox
        self.ship = ship
        
        self.hud = hud.Hud(self)
        self.input = input.Input(self)
#        self.mainmenu = mainmenu.MainMenu(self)

        self.setVelocity(10)
        self.setLasers(100)

        print "Running"
        self.app.run()


    def setVelocity(self, v):
        # Completely nonscientific equations to both set the velocity
        # of the moving texture, and stretch out the texture when going
        # fast, to give a magnified sense of motion.
        if v < 0.01:
            v = 0
        self.velocity = v
        self.world.velocity = v/10.0
        if v < 1:
            x = 1
        else:
            x = v
        self.world.vrepeat = self.world.hrepeat / x
        if hasattr(self,'hud'):
            self.hud.setVelocity(v)

    def setLasers(self, lasers):
        if hasattr(self,'hud'):
            self.hud.setLasers(lasers)
