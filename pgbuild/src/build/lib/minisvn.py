#
# minisvn.py - A minimalist Subversion client capable of doing atomic
#              checkouts, using only the Python standard library.
#
# PicoGUI Build System
# Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
# 

import httplib
import xml.dom.minidom

userAgent = "PicoGUI-MiniSVN/r$Rev$"

class MiniSvnException(Exception):
    pass

class UnknownProtocol(MiniSvnException):
    def __init__(self, name):
        self.args = name,
        self.name = name
        
class DavURL:
    """Represent an object on a WebDAV server"""
    def __init__(self, url):
        parsedURL = urlparse(url)
        if parsedURL[0] != 'http':
            raise UnknownProtocol
        self.server = parsedURL[1]
        self.path = parsedURL[2]

    def read(self):
        global userAgent
 conn.request("PROPFIND", "/svn/picogui", None,
             { 'Depth': 1,
               'User-Agent': 'PicoGUI-thingy/0.0'
               })       
        

def checkout(from, to):
    

conn = httplib.HTTPConnection(parsed_url[1])

# Retrieve all properties of this collection's children
conn.request("PROPFIND", "/svn/picogui", None,
             { 'Depth': 1,
               'User-Agent': 'PicoGUI-thingy/0.0'
               })
r1 = conn.getresponse()
print r1.status, r1.reason
data = r1.read()
conn.close()

print data

dom = xml.dom.minidom.parseString(data)

print dom
