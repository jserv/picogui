""" PGBuild.Node.XML

SCons node types for dealing with XML documents. This lets SCons track
dependencies involving individual tags or subtrees of an XML document.
Compatible with the minidom, and includes extensions like XPath.
"""
# 
# PicoGUI Build System
# Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
# 
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#  
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#  
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
# 

import SCons.Node
import PGBuild.XML.dom.minidom
import dmutil.xsl.xpath

def _needsWrapper(x):
    if type(x)==type([]) or isinstance(x, PGBuild.XML.dom.minidom.NodeList):
        return 1
    if callable(x):
        return 1

def _getNode(x):
    if isinstance(x, PGBuild.XML.dom.Node):
        if not hasattr(x, 'node'):
            # Create an Element() class if it hasn't been done
            Element(x)
        return x.node
    if type(x)==type([]) or isinstance(x, PGBuild.XML.dom.minidom.NodeList):
        out = []
        for item in x:
            out.append(_getNode(item))
        return out
    return x
    
class XPathParser:
    """Utility class to abstract the XPath implementation in use.
       Normally you should call the xpath() member of an Element
       instead of using this directly.
       """
    def __init__(self):
        self.parser = dmutil.xsl.xpath.makeParser()
        self.env = dmutil.xsl.xpath.Env()

    def parse(self, element, path):
        return self.parser(path).eval(element, [element], self.env)

default_xpath = XPathParser()

class NodeWrapper:
    """Wrapper for functions and sequences that retrieves
       the 'node' attribute of DOM elements"""
    def __init__(self, wrapped):
        self.wrapped = wrapped

    def __len__(self):
        return len(self.wrapped)

    def __getitem__(self, pos):
        # This wrapper also makes it easy to trace DOM usage for debugging :)
        #print "%s[%s]" % (self.wrapped, pos)
        return _getNode(self.wrapped[pos])

    def __call__(self, *args, **kwargs):
        #print "%s( *%s, **%s)" % (self.wrapped, args, kwargs)
        return _getNode(self.wrapped(*args, **kwargs))

class Element(SCons.Node.Node):
    """An SCons node wrapper around a DOM element object.
       The dependency checking for this object functions as if
       each XML subtree were a separate file.
       """
    def __init__(self, dom):
        SCons.Node.Node.__init__(self)
        self.dom = dom
        dom.node = self

        # Install wrapper objects that let us access the DOM methods easily
        for attr in dir(self.dom):
            if not hasattr(self, attr):
                if _needsWrapper(getattr(self.dom, attr)):
                    setattr(self, attr, NodeWrapper(getattr(self.dom, attr)))

    def __getattr__(self, attr):
        """Redirect undefined attribute requests to the DOM object"""
        return _getNode(getattr(self.dom, attr))

    def __str__(self):
        """A somewhat more helpful string representation for XML tags"""
        return "<%s.%s %s>" % (
            self.__class__.__module__,
            self.__class__.__name__,
            self.dom)

    # This node always exists, and it's outside the filesystem
    def sconsign(self):
        pass
    def exists(self):
        return 1
    def rexists(self):
        return 1
    def is_under(self, dir):
        return 1

    def get_contents(self):
        """This returns the data used for the content signature"""
        return self.toxml()

    def xpath(self, path):
        # Pass this off to the separate XPath implementation class
        return default_xpath.parse(self, path)

    def getAttrDict(self):
        """A utility to construct a python dictionary from the
           attribute name/value pairs in dom.attributes.
           """
        d = {}
        if not self.dom.attributes:
            return None
        for key in self.dom.attributes.keys():
            d[key] = self.dom.attributes[key].value
        return d

class Document(Element):
    """Read in a File node and generate a DOM
       tree populated with Element nodes.
       """
    def __init__(self, file):
        dom = PGBuild.XML.dom.minidom.parseString(file.get_contents())
        Element.__init__(self, dom)

### The End ###
        
    
