import PicoGUI
import string


class PropertyList:
    def __init__(self, app, widget, container):
        self.app = app
        self.widget = widget
        self.container = container
        self.list = []
        self.add('font')
        self.add('side')
        self.add('size')
        self.add('text')
        self.add('name')

    def add(self, property):
        self.list.append(PropertyEdit(self,property))


class PropertyEdit:
    def __init__(self, propertyList, property):
        self.plist = propertyList
        self.property = property

        if len(self.plist.list) == 0:
            self.box = self.plist.container.addWidget('box','inside')
        else:
            self.box = self.plist.list[-1].box.addWidget('box','after')
        self.box.transparent = 1

        # A checkbox displays the name and lets you enable/disable
        # the property itself and the settingsBox
        self.checkbox = self.box.addWidget('checkbox','inside')
        self.checkbox.text = string.capitalize(property)
        self.checkbox.side = 'all'
        self.plist.app.link(self._show_hide, self.checkbox, 'activate')

    def show(self):
        self.settingsBox = self.box.addWidget('box','inside')
        self.settingsBox.side = 'bottom'
        self.settingsBox.transparent = 1

        self.editWidget = self.settingsBox.addWidget('field','inside')
        self.editWidget.side = 'top'
        self.plist.app.link(self._modify, self.editWidget, 'activate')

    def hide(self):
        self.plist.app.delWidget(self.settingsBox)
        self.plist.app.delWidget(self.editWidget)

    def _show_hide(self, ev, widget):
        if widget.on:
            self.show()
        else:
            self.hide()

    def _modify(self, ev, widget):
        self.plist.widget.server.set(self.plist.widget.handle, self.property,
                                     widget.server.getstring(widget.text).data)
