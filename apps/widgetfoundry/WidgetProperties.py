import PicoGUI, string, sys

widgetTypes = {}

# Get the WidgetType class for a widget type
def getType(app, type):
    if not widgetTypes.has_key(type):
	widgetTypes[type] = WidgetType(app, type)
    return widgetTypes[type]
    

# Get a widget's property list
def getList(widget, app):
    if not hasattr(widget,'properties'):
	# We need to generate the list
	widget.properties = {}
	type = getType(app, widget.type)
	for prop in type.properties:
	    value = getattr(widget, prop)
	    if value != type.defaults[prop]:
		widget.properties[prop] = value
    return widget.properties


class WidgetType:
    """
    Gathers and stores information about the property types and
    defaults supported by a particular widget type.
    """
    def __init__(self, app, type):
        # Create a widget of this type, and scan it for settable
        # properties and their default values
        widget = app.createWidget(type)
        self.properties = []
        self.defaults = {}
        for prop in PicoGUI.server.constants['set'].keys():
            try:
                default = getattr(widget, prop)
                setattr(widget,prop,default)
                self.defaults[prop] = default
            except:
                pass
            else:
                self.properties.append(prop)
	self.properties.sort()
        app.delWidget(widget)


class PropertyList:
    "A user-editable list of widget properties"
    def __init__(self, app, widget, container):
        self.app = app
        self.widget = widget
        self.container = container
        self.list = []
	self.type = getType(app, widget.type)

        for i in self.type.properties:
            self.list.append(PropertyEdit(self,i))

    def destroy(self):
	for i in self.list:
	    i.destroy()


class PropertyEdit:
    "A generic property editor container"
    def __init__(self, propertyList, property):
        self.plist = propertyList
        self.property = property
        self.widget = self.plist.widget
        self.app = self.plist.app
	self.type = self.plist.type
	self.visible = False

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
        if self.value != self.type.defaults[self.property]:
            self.checkbox.on = 1
            self.show()
	    self.propertyValid()

    def destroy(self):
	self.deleteEditorWidgets()
	self.app.delWidget(self.checkbox)
	
    def show(self):
	if not self.visible:
	    self.editor = StringPropertyEditor(self)
	    self.visible = True

    def deleteEditorWidgets(self):
	self.editor.destroy()
        if hasattr(self,'errorLabel'):
            self.app.delWidget(self.errorLabel)	

    def hide(self):
	if self.visible:
	    self.deleteEditorWidgets()
	    self.value = self.type.defaults[self.property]
	    setattr(self.widget, self.property, self.value)
	    self.propertyInvalid()
	    self.visible = False

    def _show_hide(self, ev, widget):
        if widget.on:
            self.show()
        else:
            self.hide()

    def set(self,value):
	self.show()
	
        # Delete a stale error notice if we have one
        if hasattr(self,'errorLabel'):
	    self.app.delWidget(self.errorLabel)

        # If we have an exception setting the property,
        # slap an invalid warning on this property
        self.value = value
        try:
            setattr(self.widget, self.property, self.value)
	    if not hasattr(self.widget,'properties'):
		self.widget.properties = {}
	    self.propertyValid()
        except:
            self.errorLabel = self.box.addWidget('label','inside')
            self.errorLabel.side = 'bottom'
            self.errorLabel.text = 'Invalid Value'
	    self.propertyInvalid()

    def propertyValid(self):
	self.widget.properties[self.property] = self.value

    def propertyInvalid(self):
	if hasattr(self.widget,'properties'):
	    if self.widget.properties.has_key(self.property):
		del self.widget.properties[self.property]


class StringPropertyEditor:
    "An editor for properties that can be represented as strings"
    def __init__(self,propertyEdit):
	self.pedit = propertyEdit
	self.app = self.pedit.app

	self.editWidget = self.pedit.box.addWidget('field','inside')
        self.editWidget.side = 'bottom'
        self.editWidget.text = str(self.pedit.value)
        self.app.link(self._modify, self.editWidget, 'activate')

    def _modify(self, ev, widget):
	self.pedit.set(widget.text)

    def destroy(self):
        self.app.delWidget(self.editWidget)
