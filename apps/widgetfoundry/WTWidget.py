import PicoGUI, string, sys


class Widget:
    """
    A wrapper around the PicoGUI widget that is selectable, stores
    properties more reliably, can enumerate its supported
    properties and defaults, and dump itself to XWT format.
    """
    def __init__(self,app,pgwidget):
        self.pgwidget = pgwidget
        self.app = app

        # Get a WidgetType class for this widget.
        # Cache the WidgetType objects per-app.
        if not hasattr(app,'widgetTypes'):
            app.widgetTypes = {}
        type = pgwidget.type
        if not app.widgetTypes.has_key(type):
           app. widgetTypes[type] = WidgetType(app, type)
        self.type = app.widgetTypes[type]

        self.properties = {}
        self.children = []

    # Settings for outputting XWTs
    indentSize = 4
    maxInlineLength = 70

    def getIndent(self, indentLevel):
        return ' ' * (self.indentSize * indentLevel)

    def unset(self, property):
        del self.properties[property]
        setattr(self.pgwidget,property,self.type.defaults[property])

    def set(self, property, value):
        self.properties[property] = value
        setattr(self.pgwidget,property,value)

    # Returns the attribute list, with separator before each pair
    def getAttributes(self, properties=None, separator=' '):
        attr = ''
        if properties == None:
            properties = self.properties
        for prop in properties:
            attr += '%s%s="%s"' % (separator, prop, self.properties[prop])
        return attr

    def toXWT(self, indentLevel=0):
        indent = self.getIndent(indentLevel)
        properties = self.properties.copy()  # Copy it so we can remove 'text' below
        
        # Text goes in the tag's data unless there are children
        if len(self.children) == 0 and properties.has_key('text'):
            data = properties['text']
            del properties['text']
        else:
            data = ''
            
        xwt = "%s<%s%s" % (indent, self.type.name, self.getAttributes(properties))
            
        # Was that too long? We can put properties on individual lines
        if len(xwt) > self.maxInlineLength:
            separator = '\n' + getIndent(indentLevel+1)
            xwt = '%s<%s%s%s' % (indent, self.type.name,
                                 self.getAttributes(properties,separator), separator)
            
        # If we have no widgets or data, make this a self-closing tag and exit
        if len(self.children) == 0 and len(data) == 0:
            return xwt + '/>\n'
        else:
            xwt += '>\n'

        if len(data) > 0:
            xwt += self.getIndent(indentLevel+1) + data + '\n'

        for child in self.children:
            xwt += child.toXWT(indentLevel+1)

        return '%s%s</%s>\n' % (xwt, indent, self.type.name)


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
        self.name = type
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

        for i in widget.type.properties:
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

        if self.widget.properties.has_key(self.property):
            self.checkbox.on = 1
            self.show()
	    self.propertyValid()

    def destroy(self):
	self.deleteEditorWidgets()
	self.app.delWidget(self.checkbox)
	
    def show(self):
	if not self.visible:
            self.widget.set(self.property,self.widget.type.defaults[self.property])
	    self.editor = StringPropertyEditor(self)
	    self.visible = True

    def deleteEditorWidgets(self):
	self.editor.destroy()
        if hasattr(self,'errorLabel'):
            self.app.delWidget(self.errorLabel)	

    def hide(self):
	if self.visible:
	    self.deleteEditorWidgets()
            self.widget.unset(self.property)
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
        try:
            self.widget.set(self.property, value)
        except:
            self.errorLabel = self.box.addWidget('label','inside')
            self.errorLabel.side = 'bottom'
            self.errorLabel.text = 'Invalid Value'


class StringPropertyEditor:
    "An editor for properties that can be represented as strings"
    def __init__(self,propertyEdit):
	self.pedit = propertyEdit
	self.app = self.pedit.app

	self.editWidget = self.pedit.box.addWidget('field','inside')
        self.editWidget.side = 'bottom'
        self.editWidget.text = str(self.pedit.widget.properties[self.pedit.property])
        self.app.link(self._modify, self.editWidget, 'activate')

    def _modify(self, ev, widget):
	self.pedit.set(widget.text)

    def destroy(self):
        self.app.delWidget(self.editWidget)

