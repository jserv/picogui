import PicoGUI
from Components import PanelComponent


class Toolbox(PanelComponent):
    "Widgets and widget properties"
    def __init__(self, main):
        self.super = PanelComponent
        self.super.__init__(self,main)
        self.widget.side = 'left'
        self.widget.text = 'Toolbox'

    def destroy(self):
        self.super.destroy(self)

