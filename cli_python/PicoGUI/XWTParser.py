#!/usr/bin/python2.2
# Parser for XML Widget Templates

import PicoGUI
from xml.parsers import expat
    
class XWTParser:
    def __init__(self):
        self.widgetStack = []
    
    def __startElementHandler(self, name, attrs):
        # Is this widget at the root level? If this is an application
        # of some type, we'll create that, otherwise make a detached widget.
        if len(self.widgetStack) == 0:
            if name == 'application' or name == 'toolbarapp':
                if attrs.has_key('text'):
                    apptitle = attrs['text']
                    del attrs['text']
                else:
                    apptitle = ''
                if name == 'toolbarapp':
                    self.app = PicoGUI.ToolbarApp(apptitle,PicoGUI.WTFile())
                else:
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

        # All remaining XML attributes are widget properties.
        # Any attribute ending with "_src" is considered to be a filename
        # pointing to the property data rather than the property data itself.
        for property in attrs.keys():
            value = attrs[property]
            if property[-4:] == '_src':
                value = open(value,'r').read()
                property = property[:-4]
            canonicalProperty = property.lower().replace('_', ' ')
            widget.server.set(widget.handle, canonicalProperty, value)
        
        widget.children = []
        self.widgetStack.append(widget)

    def __endElementHandler(self, name):
        self.widgetStack = self.widgetStack[:-1]

    def __characterDataHandler(self, data):
        widget = self.widgetStack[-1]
        data = data.strip()
        if len(data):
            widget.server.set(widget.handle, 'text', data)

    def Parse(self,xwt):
        Parser = expat.ParserCreate()
        Parser.StartElementHandler = self.__startElementHandler
        Parser.EndElementHandler = self.__endElementHandler
        Parser.CharacterDataHandler = self.__characterDataHandler
        Parser.Parse(xwt)
        return self.app.server.dump()

# execute from command line, if wished
if __name__ == '__main__':
    import sys
    if len(sys.argv) != 2:
        print 'usage: %s FILE.XWT' % sys.argv[0]
        sys.exit(0)
    parser = XWTParser()
    sys.stdout.write(parser.Parse(file(sys.argv[1]).read()))

### The End ###
