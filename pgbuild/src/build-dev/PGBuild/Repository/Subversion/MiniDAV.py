""" PGBuild.Repository.Subversion.MiniDAV

A minimal read-only WebDAV client
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
_svn_id = "$Id$"

from httplib import HTTPConnection
from urlparse import urlparse, urlunparse
import sys
import PGBuild.XMLUtil
import PGBuild.Errors

try:
    revision = "$Rev: 4078 $".split()[1]
except IndexError:
    revision = None

userAgent = "%s/%s" % (PGBuild.name, PGBuild.version)

class ResponseError(PGBuild.Errors.ExternalError):
    explanation = "There was a problem parsing the Subversion server's PROPFIND response"
    def __init__(self, args=None):
        self.args = args

class DavObject(object):
    """Represent an object on a WebDAV server"""
    def __init__(self, url):
        parsedURL = urlparse(url)
        if parsedURL[0] != 'http':
            raise PGBuild.Errors.UserError("Unknown protocol '%s'" % parsedURL[0])
        self.url = url
        self.server = parsedURL[1]
        self.path = parsedURL[2]

    def read(self):
        """Read the contents of the entire object, returning it"""
        global userAgent
        conn = HTTPConnection(self.server)
        conn.request("GET", self.path, None,
                     { 'User-Agent': userAgent,
                       })       
        resp = conn.getresponse()
        if resp.status != 200:
            raise PGBuild.Errors.EnvironmentError(
                "Server returned status '%d' for GET request" % resp.status)
        data = resp.read()
        conn.close()
        return data
        
    def propfind(self):
        """Read the object's properties, storing them. This is called
           automatically once if property values are needed, but may
           be called again to invalidate the locally cached properties.
        """
        global userAgent
        conn = HTTPConnection(self.server)
        conn.request("PROPFIND", self.path, None,
                     { 'Depth': 1,
                       'User-Agent': userAgent,
                       })       
        resp = conn.getresponse()
        if resp.status != 207:
            raise PGBuild.Errors.EnvironmentError(
                "Server returned status '%d' for PROPFIND request" % resp.status)
            raise ErrorResponse(resp)
        try:
            dom = PGBuild.XMLUtil.Document(resp)
        except:
            raise PGBuild.Errors.ResponseError(
                "The response does not appear to be valid XML\n%s %s" %
                (sys.exc_info()[0], sys.exc_info()[1]))
        conn.close()

        # Now parse up our response. We should have a <DAV::multistatus> at the root,
        # with a <DAV::response> for each child and ourselves.
        try:
            root = dom.getElementsByTagNameNS("DAV:", "multistatus")[0]
        except IndexError:
            raise PGBuild.Errors.ResponseError(
                "A DAV <multistatus> element was not found at the response's root")

        # Parse the <response> tags into a hash associating URI with <prop> trees
        responses = {}
        for response in root.getElementsByTagNameNS("DAV:", "response"):
            try:
                uri = PGBuild.XMLUtil.getText(response.getElementsByTagNameNS("DAV:", "href")[0])
            except IndexError:
                raise PGBuild.Errors.ResponseError("An <href> tag was not found in the <response>")
            try:
                propstat = response.getElementsByTagNameNS("DAV:", "propstat")[0]
            except IndexError:
                raise PGBuild.Errors.ResponseError("An <propstat> tag was not found in the <response>")
            try:
                prop = propstat.getElementsByTagNameNS("DAV:", "prop")[0]
            except IndexError:
                raise PGBuild.Errors.ResponseError("An <prop> tag was not found in the <propstat>")
            responses[uri] = prop

        # We now have a dictionary of the responses, keyed by the href.
        # First extract the response for our own properties.
        # If the URL we were given didn't have a trailing slash even though
        # it was a collection, it won't match exactly.
        self.properties = None
        for response in responses:
            if response == self.path or response == self.path + '/':
                # We can tell based on whether this response has a trailing slash,
                # if this object is a collection or a file.
                if response[-1] == '/':
                    self.type = 'collection'
                else:
                    self.type = 'file'

                self.properties = responses[response]
                del responses[response]
                break
        if not self.properties:
            raise PGBuild.Errors.ResponseError(
                "No <response> received with an <href> corresponding to the requested URI")

        # All the remaining <response>'s define children, so create DavObjects for them
        # and file the property trees away appropriately.
        self.children = []
        for response in responses:
            obj = DavObject(urlunparse(('http', self.server, response, '', '', '')))
            obj.properties = responses[response]

            # As above, we can tell from the response whether the child is a collection
            # or not. If not, go ahead and give it an empty children list.
            if response[-1] == '/':
                obj.type = 'collection'
            else:
                obj.type = 'file'
                obj.children = []

            self.children.append(obj)

    def getChildren(self):
        if not hasattr(self, "children"):
            self.propfind()
        return self.children

    def getPropertiesDOM(self):
        """Return the DOM representation of all properties"""
        if not hasattr(self, "properties"):
            self.propfind()
        return self.properties

    def getPropertyDOM(self, name, namespace="DAV:"):
        """Return the DOM representation of one property"""
        props = self.getPropertiesDOM()
        if namespace:
            prop = props.getElementsByTagNameNS(namespace, name)
        else:
            prop = props.getElementsByTagName(name)
        if not prop:
            return None
        return prop[0]

    def getPropertyText(self, name, namespace="DAV:"):
        """Return all text contained within a property"""
        return PGBuild.XMLUtil.getTextRecursive(self.getPropertyDOM(name, namespace)).strip()

    def getPropertyDict(self):
        """Return a dictionary mapping element names of the form
           namespaceURI:localName to the property's full text.
           """
        pdict = {}
        for child in self.getPropertiesDOM().childNodes:
            if child.nodeType == child.ELEMENT_NODE:
                name = "%s:%s" % (child.namespaceURI, child.localName)
                pdict[name] = PGBuild.XMLUtil.getTextRecursive(child).strip()
        return pdict

    def getType(self):
        if not hasattr(self, "type"):
            self.propfind()
        return self.type

### The End ###
