import PicoGUI, string, sys


class PropertyList:
    def __init__(self, app, widget, container):
        self.app = app
        self.widget = widget
        self.container = container
        self.list = []

	widget.properties = ('text','font','side','thobj','transparent')

	# Take the widget's current state as default if
	# we haven't already saved other defaults.
	if not hasattr(widget,'defaults'):
		widget.defaults = {}
		for i in widget.properties:
			widget.defaults[i] = getattr(widget,i)

	for i in widget.properties:
		widget.defaults
	        self.add(i)

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

        self.value = getattr(self.widget, self.property)
	if self.value != self.widget.defaults[self.property]:
		self.checkbox.on = 1
		self.show()

    def show(self):
        self.settingsBox = self.box.addWidget('box','inside')
        self.settingsBox.side = 'bottom'
        self.settingsBox.transparent = 1

        self.editWidget = self.settingsBox.addWidget('field','inside')
        self.editWidget.side = 'top'
        self.editWidget.text = str(self.value)
        self.app.link(self._modify, self.editWidget, 'activate')

    def hide(self):
        self.app.delWidget(self.settingsBox)
        self.app.delWidget(self.editWidget)
        if hasattr(self,'errorLabel'):
            self.app.delWidget(self.errorLabel)
	self.value = self.widget.defaults[self.property]
	setattr(self.widget, self.property, self.value)

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
