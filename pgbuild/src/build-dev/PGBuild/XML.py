""" PGBuild.XML

A portability wrapper around Python's XML modules
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

# A little explanation is in order for this code.
# As of this writing, Gentoo Linux includes a broken _xmlplus module.
# This wouldn't be so bad, since we only need core XML functionality,
# except that the xml module transparently replaces itself with
# _xmlplus if it's available. See this writeup on the "_xmlplus hack":
#    http://www.amk.ca/conceit/xmlplus.html
#
# So, in the interest of portability, (and so I don't have to wait for
# Gentoo to fix the bug) this wrapper exists. It performs a simple test
# of the xml module, and if it fails it uses a similarly hackish trick
# to prevent the xml module from loading _xmlplus.

class TestFailed(Exception):
    pass

def test():
    """A very flimsy test to determine if there's any chance of the current
       XML implementation functioning correctly.
       """
    
    import xml.dom.minidom

    document = """\
    <ramses>
       <niblick/>
       <niblick/>
       <niblick>
         <kerplunk/>
         <kerplunk>
           Whoops, where's my thribble?
         </kerplunk>
       </niblick>
    </ramses>
    """

    dom = xml.dom.minidom.parseString(document)

    for a in dom.getElementsByTagName("ramses"):
        for b in a.getElementsByTagName("niblick"):
            for c in b.getElementsByTagName("kerplunk"):
                for d in c.childNodes:
                    if d.nodeType == d.TEXT_NODE:
                        if d.toxml().find("thribble") == 30:
                            return 1
    raise TestFailed()


# Try the test using the default XML modules
try:
    test()
except:
    # Bah, that doesn't work. Now let's forcibly unload all the xml modules,
    # delete xmlplus's version so xml won't try to use it, and run the test again.
    import sys, _xmlplus
    for m in filter(lambda x: x.find('xml')==0, sys.modules):
        del sys.modules[m]
    del _xmlplus.version_info
    test()
    
# Assuming all that worked, point this module object at the xml
# module we just loaded, as a convenience.
# This lets us import modules like PGBuild.XML.dom.minidom
import sys, xml
sys.modules[__name__] = xml

### The End ###
        
    
