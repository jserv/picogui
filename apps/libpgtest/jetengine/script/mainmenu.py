import PicoGUI

class MainMenu:
    def __init__(self, game):
        self.game = game
        self.app = game.app
        
        # Load our widget template, importing widgets from it
        self.wt = self.app.newTemplate(open("data/mainmenu.wt").read()).instantiate([
            ])

