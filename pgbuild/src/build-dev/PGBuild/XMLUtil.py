""" PGBuild.XMLUtil

Utilities to make dealing with XML easier. Includes an easily
subclassable document class, an SCons Node for XML elements,
and XPath utilities.
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

class Node(SCons.Node.Node):
    """An SCons node wrapper around a DOM element object.
       The dependency checking for this object functions as if
       each XML subtree were a separate file.
       """
    def __init__(self, dom):
        SCons.Node.Node.__init__(self)
        self.dom = dom

    def __str__(self):
        """A somewhat more helpful string representation for XML tags"""
        return "<%s.%s %s>" % (
            self.__class__.__module__,
            self.__class__.__name__,
            self.dom)

    def __repr__(self):
        return str(self)

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
        return self.dom.toxml()

def xpath(context, path):
    """Convenience function for parsing XPaths"""
    return default_xpath.parse(context, path)

def getAttrDict(element):
    """A utility to construct a python dictionary from the
       attribute name/value pairs in dom element attributes.
       """
    d = {}
    if not element.attributes:
        return None
    for key in element.attributes.keys():
        d[key] = element.attributes[key].value
    return d

class Document(PGBuild.XML.dom.minidom.Document):
    """Class that abstracts DOM-implementation-specific details on
       document loading. This accepts several types of input, in
       order of attempt:

       1. A class with a get_contents() method, such as an SCons file node
       2. A file object
       3. A string, (or string-like object) treated as the XML data to parse

       This class also provides a few convenience functions, and makes
       it easier to subclass an XML document.
       """
    def __init__(self, input):
        try:
            dom = PGBuild.XML.dom.minidom.parseString(input.get_contents())
        except AttributeError:
            if type(input) == file:
                dom = PGBuild.XML.dom.minidom.parse(input)
            else:
                dom = PGBuild.XML.dom.minidom.parseString(input)
                
        # Copy the attributes from the loaded DOM object into ourselves, being
        # careful not to overwrite any existing attributes.
        # This lets us subclass the minidom Document but still initialize
        # it via a factory function.
        for attr in dir(dom):
            if not hasattr(self, attr):
                setattr(self, attr, getattr(dom, attr))

    def getRoot(self):
        """Return the document's root element, caching
           it in the 'root' attribute.
           """
        if not hasattr(self, 'root'):
            for n in self.childNodes:
                if n.nodeType == n.ELEMENT_NODE:
                    self.root = n
                    break
        return self.root

    def xpath(self, path, context=None):
        """Another XPath convenience function.
           By default, paths are relative to the document's root element.
           """
        if not context:
            context = self.getRoot()
        return default_xpath.parse(context, path)

    def node(self, path=None, context=None):
        """For convenience, an XMLUtil.Node factory. This by default
           refers to the document root, but an XPath can be given to
           cause it to refer to any arbitrary subtree. If the XPath
           matches exactly once, a single node will be returned. If
           it doesn't match at all, this returns None. If it matches
           multiple times, this returns a list of nodes.
           """
        if path:
            n = self.xpath(path, context)
        else:
            n = [self]
        if len(n) == 0:
            return None
        for i in xrange(len(n)):
            # Note: We could cache the resulting SCons nodes, but
            #       it would create circular references. They're small
            #       enough it's really not worth the trouble.
            n[i] = Node(n[i])
        if len(n) == 1:
            return n[0]
        return n
    
### The End ###
        
    