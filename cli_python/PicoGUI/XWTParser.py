# Parser for XML Widget Templates

import PicoGUI, string
from xml.parsers import expat
    
class XWTParser:
    def __init__(self):
        self.widgetStack = []
    
    def __startElementHandler(self, name, attrs):
        # Is this widget at the root level? If this is an application
        # of some type, we'll create that, otherwise make a detached widget.
        if len(self.widgetStack) == 0:
            if name == 'application':
                if attrs.has_key('text'):
                    apptitle = attrs['text']
                    del attrs['text']
                else:
                    apptitle = ''
                self.app = PicoGUI.Application(apptitle,PicoGUI.WTFile())
                widget = self.app
            else:
                self.app = PicoGUI.InvisibleApp('',PicoGUI.WTFile())
                widget = self.app.createWidget(name)
                
        # Is it the first child widget in this parent?
        elif len(self.widgetStack[-1].children) == 0:
            widget = self.widgetStack[-1].addWidget(name,'inside')
            self.widgetStack[-1].children.append(widget)

        # Or just an additional child?
        else:
            widget = self.widgetStack[-1].children[-1].addWidget(name,'after')
            self.widgetStack[-1].children.append(widget)

        # All remaining XML attributes are widget properties
        for property in attrs.keys():
            widget.server.set(widget.handle, property, attrs[property])
        
        widget.children = []
        self.widgetStack.append(widget)

    def __endElementHandler(self, name):
        self.widgetStack = self.widgetStack[:-1]

    def __characterDataHandler(self, data):
        widget = self.widgetStack[-1]
        data = string.strip(data)
        if len(data):
            widget.server.set(widget.handle, 'text', data)

    def Parse(self,xwt):
        Parser = expat.ParserCreate()
        Parser.StartElementHandler = self.__startElementHandler
        Parser.EndElementHandler = self.__endElementHandler
        Parser.CharacterDataHandler = self.__characterDataHandler
        Parser.Parse(xwt)
        return self.app.server.dump()

### The End ###
