import PicoGUI

class Hud:
    def __init__(self, game):
        self.game = game
        self.app = game.app
        
        # Load our widget template, importing widgets from it
        self.wt = self.app.newTemplate(open("data/hud.wt").read()).instantiate([
            'Velocity',
            'Lasers',
            ])

    def setVelocity(self, v):
        self.wt.Velocity.text = "%.0f" % v

    def setLasers(self, lasers):
        self.wt.Lasers.text = str(lasers)
                
