import PicoGUI
from Components import PanelComponent
from WTWidget import PropertyList

    
class Page:
    """
    One mutually exclusive page in the toolbox. Once pgserver
    supports a real tab widget this should be modified to use that.
    """
    def _showHide(self, ev, button):
        for page in self.toolbox.pages:
            page.hide()
        if button.on:
            self.show()

    def __init__(self, toolbox, name):
        self.toolbox = toolbox
        self.main = toolbox.main
        self.button = toolbox.toolbar.addWidget('button','inside')
        self.button.extdevents = ('exclusive','toggle')
        self.button.text = name
        self.box = toolbox.toolbar.addWidget('box')
        self.box.transparent = 1
        self.box.margin = 0
        self.scroll = self.box.addWidget('scrollbox','inside')
        self.main.app.link(self._showHide, self.button, 'activate')
        self.hide()

    def show(self):
        self.box.side = 'all'
        self.visible = 1

    def hide(self):
        self.box.side = 'top'
        self.box.size = 0
        self.visible = 0

    def destroy(self):
        self.main.app.delWidget(self.button)
        self.main.app.delWidget(self.box)
        self.main.app.delWidget(self.scroll)
        

class PropertyPage(Page):
    "List of properties for the selected widget"
    def __init__(self, toolbox):
        Page.__init__(self, toolbox, 'Properties')
        self.info = self.box.addWidget('label','inside')
        self.info.transparent = 0
        self.info.align = 'left'
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
        Page.destroy(self)


class XWTPage(Page):
    "Viewer for the current XWT tree"
    def __init__(self, toolbox):
        Page.__init__(self, toolbox, 'XWT')
        self.main.changeNotify[self] = self.changeNotify
        self.viewer = self.scroll.addWidget('textbox','inside')
        self.viewer.side = 'all'
        self.viewer.readonly = 1

    def changeNotify(self, widget, property):
        if self.visible:
            self.update()

    def show(self):
        self.update()
        Page.show(self)

    def update(self):
        xwt = self.main.workArea.root.toXWT()
        self.viewer.text = xwt

    def destroy(self):
        del self.main.changeNotify[self]
        self.main.app.delWidget(self.textbox)
        Page.destroy(self)


class Toolbox(PanelComponent):
    "Widgets and widget properties"

    pageClasses = [PropertyPage, XWTPage]

    def __init__(self, main):
        PanelComponent.__init__(self,main)
        self.widget.side = 'left'
        self.widget.text = 'Toolbox'
        self.toolbar = self.widget.addWidget('toolbar','inside')

        pageClasses = self.pageClasses[:]
        pageClasses.reverse()
        self.pages = []
        for pageClass in pageClasses:
            self.pages.append(pageClass(self))

    def destroy(self):
        for page in self.pages:
            page.destroy()
        self.main.app.delWidget(self.toolbar)
        PanelComponent.destroy(self)
