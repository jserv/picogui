"""
 MiniSVN.py - A minimalist Subversion client capable of doing
              multithreaded atomic checkouts, using only the
              Python standard library.
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

import httplib
import urlparse
import xml.parsers.expat

rev = "$Rev$"
userAgent = "PicoGUI-MiniSVN/r" + rev.split()[1]


class MiniSvnException(Exception):
    pass

class UnknownProtocol(MiniSvnException):
    def __init__(self, name):
        self.args = name,
        self.name = name

class ErrorResponse(MiniSvnException):
    def __init__(self, resp):
        self.args = "%s %s" % (resp.status, resp.reason),
        self.status = resp.status
        self.reason = resp.reason

class InvalidDavResponse(MiniSvnException):
    pass


class DavPropertyParser:
    """Utility to parse the XML responses from a PROPFIND request"""
    def __init__(self):
        self.elementStack = []
        self.responses = {}
        self.currentResponse = None
        self.currentProperty = None
    
    def __startElementHandler(self, name, attrs):
        self.elementStack.append(name)
        if name == 'DAV::response':
            self.currentResponse = {}

        # Consider a tag immediately inside a <D:prop> to be a property
        try:
            if self.elementStack[-2] == 'DAV::prop':
                self.currentProperty = self.elementStack[-1]
        except IndexError:
            pass

        # If we get a tag inside a property tag, it might be worth saving. If we also
        # get data, as in the case of <D:href>, the data will overwrite this. But, if we only
        # have a tag this will end up storing the tag name as the property value. This is a
        # hackish but painless way to make tags like <D:collection/> visible.
        try:
            if self.elementStack[-3] == 'DAV::prop':
                self.currentResponse[self.currentProperty] = self.elementStack[-1]
        except IndexError:
            pass
        
            
    def __endElementHandler(self, name):
        if self.elementStack.pop() != name:
            raise InvalidDavResponse()

        # If we just left a tag immediately inside the <D:prop>, clear the current property
        try:
            if self.elementStack[-1] == 'DAV::prop':
                self.currentProperty = None
        except IndexError:
            pass

    def __characterDataHandler(self, data):
        # The href tag sent immediately inside a new response tag will give the response
        # its name in our dictionary
        if self.elementStack[-1] == 'DAV::href' and self.elementStack[-2] == 'DAV::response':
            self.responses[data] = self.currentResponse

        # If we think we're inside a property, stow this data
        if self.currentProperty:
            self.currentResponse[self.currentProperty] = data

    def parse(self, data):
        Parser = xml.parsers.expat.ParserCreate("UTF-8", ":")        
        Parser.StartElementHandler = self.__startElementHandler
        Parser.EndElementHandler = self.__endElementHandler
        Parser.CharacterDataHandler = self.__characterDataHandler
        Parser.Parse(data)

        
class DavObject:
    """Represent an object on a WebDAV server"""
    def __init__(self, url):
        parsedURL = urlparse.urlparse(url)
        if parsedURL[0] != 'http':
            raise UnknownProtocol(parsedURL[0])
        self.url = url
        self.server = parsedURL[1]
        self.path = parsedURL[2]

    def read(self):
        """Read the contents of the entire object, returning it"""
        global userAgent
        conn = httplib.HTTPConnection(self.server)
        conn.request("GET", self.path, None,
                     { 'User-Agent': userAgent,
                       })       
        resp = conn.getresponse()
        if resp.status != 200:
            raise ErrorResponse(resp)
        data = resp.read()
        conn.close()
        return data
        
    def propfind(self):
        """Read the object's properties, storing them. This is called
           automatically once if property values are needed, but may
           be called again to invalidate the locally cached properties.
        """
        global userAgent
        conn = httplib.HTTPConnection(self.server)
        conn.request("PROPFIND", self.path, None,
                     { 'Depth': 1,
                       'User-Agent': userAgent,
                       })       
        resp = conn.getresponse()
        if resp.status != 207:
            raise ErrorResponse(resp)
        parser = DavPropertyParser()
        parser.parse(resp.read())
        conn.close()

        # We now have a dictionary of the responses, keyed by the href.
        # If the URL we were given didn't have a trailing slash even though
        # it was a collection, it won't match exactly.
        self.properties = None
        for response in parser.responses.keys():
            if response == self.path or response == self.path + '/':
                self.properties = parser.responses[response]
                del parser.responses[response]
        if not self.properties:
            raise InvalidDavResponse()

        # Now that the response for this object has been safely tucked away,
        # all the objects left in the list, if any, should be subdirectories.
        # Convert each one of them to a DavObject, and store the properties
        # since we already have them. Note that this leaves the children's
        # children properties undefined.
        self.children = []
        for response in parser.responses.keys():
            obj = DavObject(urlparse.urlunparse(('http', self.server, response, '', '', '')))
            obj.properties = parser.responses[response]

            # If the child isn't a collection, give it an empty children list to
            # save us from having to call propfind() to figure that out later.
            if hasattr(obj.properties, 'DAV::resourcetype'):
                if obj.properties['DAV::resourcetype'] != 'DAV::collection':
                    obj.children = []
            else:
                obj.children = []

            self.children.append(obj)

    def getChildren(self):
        if not hasattr(self, "children"):
            self.propfind()
        return self.children

    def getProperties(self):
        if not hasattr(self, "properties"):
            self.propfind()
        return self.properties


if __name__ == '__main__':
    import sys
    obj = DavObject(sys.argv[1])
    print "\nURL: %s" % obj.url

    print "\nChildren:"
    for child in obj.getChildren():
        print "\t%s" % child.path

    print "\nProperties:"
    for property in obj.getProperties():
        print "\t%s = %s" % (property, obj.getProperties()[property])
