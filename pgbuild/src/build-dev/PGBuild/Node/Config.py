""" PGBuild.Node.Config

Implements PGBuild's configuration tree, formed from multiple XML
documents with one extra restriction placed on them: Any elements
with the same parent, name, and attributes, are treated as equal.
They will be merged, with child nodes appended.

The configuration document is based on PGBuild.Node.XML, so its
elements are both SCons nodes and XML DOM elements. The configuration
document is created by 'mounting' XML files. A read-only mount will
have no further effect after its contents are read and merged with
the config document. A read-write mount will be used to save a
modified configuration document.
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

import PGBuild.Node.XML

class DefaultDocument:
    """Default contents of the config tree, passable to
       PGBuild.Node.XML.Document()
       """
    def get_contents(self):
        return "<pgbuild/>"

def merge(self, element):
    """Merge all identical elements under the given one,
       as defined in this module's document string.
       """
    pass

class MountedDocument:
    """Object defining the file and mode of a mounted
       document, attached to each node in the tree.
       """
    def __init__(self, file, mode):
        self.file = file
        self.mode = mode

class Tree(PGBuild.Node.XML.Document):
    """Configuration tree- an XML document with a <pgbuild> root
       that supports merging in other documents with a <pgbuild>
       root, then saving changes back to those documents.
       """
    def __init__(self):
        PGBuild.Node.XML.Document(DefaultDocument())

    def mount(self, file, mode="r"):
        """Mount the given document in the config tree, at the
           location specified in that document's pgbuild/@root.
           The default mode of "r" prevents writing changes back,
           a mode of "w" allows writing back changes.
           """
        mdoc = MountedDocument(file, mode)
        dom = PGBuild.Node.XML.Document(file)

        # Recursively tag all objects in the
        # new DOM with their MountedDocument
        def rTag(element, mdoc):
            element.mdoc = mdoc
            for child in element.childNodes:
                rTag(child, mdoc)
        rTag(dom, mdoc)

default = Tree()

### The End ###
