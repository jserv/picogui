# $Id$
#
# WTWidget.py - Extensions to the PicoGUI widget class to support
#               Widget Foundry's XWT exporting and property editing
#
# PicoGUI small and efficient client/server GUI
# Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
import PicoGUI, string, sys


class Widget:
    """
    A wrapper around the PicoGUI widget that is selectable, stores
    properties more reliably, can enumerate its supported
    properties and defaults, and dump itself to XWT format.
    """
    def __init__(self,app,pgwidget):
        # Get a WidgetType class for this widget.
        # Cache the WidgetType objects per-app.
        if not hasattr(app,'widgetTypes'):
            app.widgetTypes = {}
        type = pgwidget.type
        if not app.widgetTypes.has_key(type):
           app.widgetTypes[type] = WidgetType(app, type)

        # This avoids calling __setattr__
        self.__dict__.update({
            'pgwidget': pgwidget,
            'app': app,
            'properties': {},
            'children': [],
            'wtype': app.widgetTypes[type],
            'parent': None,
           })

    def changeNotify(self, widget, property):
        if self.parent:
            self.parent.changeNotify(widget,property)

    def __eq__(self, other):
        return self.pgwidget == other.pgwidget

    def __delattr__(self, name):
        # Standard picogui widgets don't support deleting properties.
        # Here we delete it from our list and set the pgwidget's property
        # to what we think is the default.
        pname = name.lower().replace('_', ' ')
        del self.properties[pname]
        setattr(self.pgwidget,pname,self.wtype.defaults[pname])
        self.changeNotify(self,pname)

    def __setattr__(self, name, value):
        pname = name.lower().replace('_', ' ')
        if pname in self.wtype.properties:
            # Set it in our list and the pgwidget
            self.properties[pname] = value
            setattr(self.pgwidget,pname,value)
            self.changeNotify(self,pname)
        else:
            self.__dict__[name] = value
            
    def __getattr__(self, name):
        pname = name.lower().replace('_', ' ')
        # Our list is considered authoritative when getting properties
        return self.properties[pname]
        
    def addWidget(self, wtype, relationship=None):
        # Wrap the new widget, and maintain the child/parent attributes
        if not relationship:
            relationship = self.pgwidget.default_relationship
        newWidget = Widget(self.app,self.pgwidget.addWidget(wtype,relationship))

        if relationship == 'inside':
            self.children = [newWidget] + self.children
            newWidget.parent = self
        elif relationship == 'after':
            newWidget.parent = self.parent
            if self.parent:
                self.parent.children.insert(self.parent.children.index(self)+1,newWidget)
        elif relationship == 'before':
            newWidget.parent = self.parent
            if self.parent:
                self.parent.children.insert(self.parent.children.index(self),newWidget)

        return newWidget

    # Settings for outputting XWTs
    indentSize = 4
    maxInlineLength = 70

    def getIndent(self, indentLevel):
        return ' ' * (self.indentSize * indentLevel)

    # Returns the attribute list, with separator before each pair
    def getAttributes(self, properties=None, separator=' '):
        attr = ''
        if properties == None:
            properties = self.properties
        keys = properties.keys()
        keys.sort()
        for prop in keys:
            attr += '%s%s="%s"' % (separator, prop, properties[prop])
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
            
        xwt = "%s<%s%s" % (indent, self.wtype.name, self.getAttributes(properties))
            
        # Was that too long? We can put properties on individual lines
        if len(xwt) > self.maxInlineLength:
            separator = '\n' + self.getIndent(indentLevel+1)
            xwt = '%s<%s%s%s' % (indent, self.wtype.name,
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

        return '%s%s</%s>\n' % (xwt, indent, self.wtype.name)


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
        for prop in PicoGUI.constants._constants['set'].keys():
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
        
        # Overrides for defaults we don't detect right
        self.defaults['size'] = None


class PropertyList:
    "A user-editable list of widget properties"
    def __init__(self, app, widget, container):
        self.app = app
        self.widget = widget
        self.container = container
        self.list = []

        for i in widget.wtype.properties:
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

    def destroy(self):
	self.deleteEditorWidgets()
	self.app.delWidget(self.checkbox)
	self.app.delWidget(self.box)
	
    def show(self):
	if not self.visible:
            # Set the property to the default if it's not set yet
            if not self.widget.properties.has_key(self.property):
                setattr(self.widget,self.property,
                        self.widget.wtype.defaults[self.property])
	    self.editor = StringPropertyEditor(self)
	    self.visible = True

    def deleteEditorWidgets(self):
        if hasattr(self,'editor'):
            self.editor.destroy()
        if hasattr(self,'errorLabel'):
            self.app.delWidget(self.errorLabel)	

    def hide(self):
	if self.visible:
	    self.deleteEditorWidgets()
            delattr(self.widget,self.property)
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
            setattr(self.widget, self.property, value)
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

