import PicoGUI, string, sys


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
        self.add('sizemode')
        self.add('align')
        self.add('bitmap')
        self.add('bitmask')
        self.add('bitmapside')
        self.add('margin')
        self.add('extdevents')
        self.add('on')
        self.add('disabled')
        self.add('thobj')
        self.add('thobj button')
        self.add('thobj button hilight')
        self.add('thobj button on')
        self.add('thobj button on nohilight')
        self.add('hotkey')
        self.add('hotkey flags')
        self.add('hotkey consume')
        self.add('transparent')
        self.add('color')
        self.add('direction')
        self.add('lgop')
        self.add('spacing')
        self.add('scroll x')
        self.add('scroll y')
        self.add('publicbox')
        self.add('bind')
        self.add('triggermask')
        self.add('hilighted')
        self.add('auto orientation')

    def add(self, property):
        self.list.append(PropertyEdit(self,property))


class PropertyEdit:
    def __init__(self, propertyList, property):
        self.plist = propertyList
        self.property = property
        self.widget = self.plist.widget
        self.app = self.plist.app

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
        self.app.link(self._show_hide, self.checkbox, 'activate')

    def show(self):
        self.settingsBox = self.box.addWidget('box','inside')
        self.settingsBox.side = 'bottom'
        self.settingsBox.transparent = 1

        self.value = getattr(self.widget, self.property)
        self.editWidget = self.settingsBox.addWidget('field','inside')
        self.editWidget.side = 'top'
        self.editWidget.text = str(self.value)
        self.app.link(self._modify, self.editWidget, 'activate')

    def hide(self):
        self.app.delWidget(self.settingsBox)
        self.app.delWidget(self.editWidget)
        if hasattr(self,'errorLabel'):
            self.app.delWidget(self.errorLabel)

    def _show_hide(self, ev, widget):
        if widget.on:
            self.show()
        else:
            self.hide()

    def _modify(self, ev, widget):
        # Delete a stale error notice if we have one
        if hasattr(self,'errorLabel'):
                self.app.delWidget(self.errorLabel)

        # If we have an exception setting the property,
        # slap an invalid warning on this property
        self.value = widget.text
        try:
            setattr(self.widget, self.property, self.value)
            self.valid = 1
        except:
            self.errorLabel = self.settingsBox.addWidget('label','inside')
            self.errorLabel.side = 'bottom'
            self.errorLabel.text = 'Invalid Value'
            self.valid = 0
