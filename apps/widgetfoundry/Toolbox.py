import PicoGUI
from Components import PanelComponent
from WTWidget import PropertyList

    
class PropertyBox:
    "List of properties for the selected widget"
    def __init__(self, toolbox, box):
        self.toolbox = toolbox
        self.main = toolbox.main
        self.box = box
        self.info = self.box.addWidget('label','inside')
        self.info.transparent = 0
        self.info.align = 'left'
        self.scroll = self.info.addWidget('scrollbox')
        self.main.selectNotify[self] = self.selectNotify
        self.main.changeNotify[self] = self.changeNotify
        self.selectNotify(None, self.main.selection)

    def selectNotify(self, previous, current):
        if hasattr(self,'propList'):
            self.propList.destroy()
        if current:
            self.propList = PropertyList(self.main.app, current, self.scroll)
            self.updateInfo()

    def changeNotify(self, widget, property):
        if widget == self.main.selection and property == 'name':
            self.updateInfo()
            
    def updateInfo(self):
        if self.main.selection.properties.has_key('name'):
            name = repr(self.main.selection.name)
        else:
            name = '<anonymous>'
        self.info.text = "%s widget: %s" % (self.main.selection.wtype.name, name)

    def destroy(self):
        del self.main.selectNotify[self]
        del self.main.changeNotify[self]
        if hasattr(self,'propList'):
            self.propList.destroy()
        self.main.app.delWidget(self.scroll)
        self.main.app.delWidget(self.info)
        self.main.app.delWidget(self.box)


class Toolbox(PanelComponent):
    "Widgets and widget properties"
    def __init__(self, main):
        PanelComponent.__init__(self,main)
        self.widget.side = 'left'
        self.widget.text = 'Toolbox'
        self.propertyBox = PropertyBox(self,self.widget.addWidget('box','inside'))

    def destroy(self):
        self.propertyBox.destroy()
        PanelComponent.destroy(self)
