import PicoGUI

class Input:
    def __init__(self, game):
        self.game = game
        self.app = game.app

        # Set up an input filter to catch all events that
        # aren't absorbed by any picogui widgets.
        # This adds a filter that recieves all events and absorbs none,
        # placed immediately after the input filter that dispatches pointing
        # events to picogui widgets.
        self.app.link(self.handler, self.app.addInfilter(
            self.app.server.getresource('infilter pntr dispatch')))

    def handler(self, t, sender):
        if t.dev == 'mouse':
            if t.name == 'scrollwheel':
                self.scrollWheel(t.y)
            if t.name == 'down':
                self.fire(t.x, t.y)
            if t.name == 'move':
                self.moveMouse(t.x, t.y)


    def scrollWheel(self, y):
        self.game.setVelocity(self.game.velocity - y/20.0)

    def fire(self, x, y):
        pass

    def moveMouse(self, x, y):
        pass
        
