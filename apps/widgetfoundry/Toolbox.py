import PicoGUI
from Components import PanelComponent
from WTWidget import PropertyList


class Toolbox(PanelComponent):
    "Widgets and widget properties"
    def __init__(self, main):
        PanelComponent.__init__(self,main)
        self.main = main
        self.widget.side = 'left'
        self.widget.text = 'Toolbox'

        self.propListScroll = self.widget.addWidget('scrollbox','inside')

        main.selectNotify[self] = self.selectNotify
        self.selectNotify(None, main.selection)

    def destroy(self):
        del self.main.selectNotify[self]
        PanelComponent.destroy(self)

    def selectNotify(self, previous, current):
        if hasattr(self,'propList'):
            self.propList.destroy()
        if current:
            self.propList = PropertyList(self.main.app, current, self.propListScroll)
