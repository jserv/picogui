""" PGBuild.Config

Implements PGBuild's configuration tree, formed from multiple XML
documents with some extra rules and a 'mount point' system.

The configuration document is based on PGBuild.Node.XML, so its
elements are both SCons nodes and XML DOM elements. The configuration
document is created by 'mounting' XML files. A read-only mount will
have no further effect after its contents are read and merged with
the config document. A read-write mount will be used to save a
modified configuration document.

The extra rules imposed on the XML:

1. Any elements with the same parent, name, and attributes, are
   treated as equal. They will be merged, with child nodes appended.

2. In merging child nodes, order is preserved

3. Python attributes from the second of the two nodes are preserved
   when merging. This means that metadata like the 'mdoc' attribute
   is overwritten on new mounts. So, a read-write mount merely has
   to mention an element to claim ownership of it when it comes time
   so save changes.

4. To keep the DOM tidy, text elements have leading and trailing
   whitespace stripped. Empty text nodes are then removed.

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

import PGBuild.XMLUtil
import PGBuild.Errors
import re

def getElementSig(x):
    """Return a string signature that can be used to compare two elements"""
    return repr([x.nodeName, x.parentNode, PGBuild.XMLUtil.getAttrDict(x)])

def mergeElements(root):
    """Merge all identical elements under the given one,
       as defined in this module's document string.
       """
    # Make a dictionary of signatures to (relatively) efficiently
    # determine whether any of our children are duplicates.
    d = {}
    for child in root.childNodes:
        sig = getElementSig(child)
        print sig
        if d.has_key(sig):
            # This is a duplicate! Prepend all the children from
            # the first one, and delete it.
            old = d[sig]
            while old.childNodes:
                last = old.lastChild
                if child.childNodes:
                    child.insertBefore(last, child.childNodes[0])
                else:
                    child.appendChild(last)
            #old.parentNode.removeChild(old)
        d[sig] = child
    del d

    # After all the merging at this level, traverse depth-first
    for child in root.childNodes:
        mergeElements(child)

def stripElements(root):
    """Recursively trim whitespace from text elements"""
    for child in root.childNodes:
        if child.nodeType == child.TEXT_NODE:
            child.data = child.data.strip()
        else:
            stripElements(child)

class MountInfo(PGBuild.XMLUtil.Document):
    """Object defining the file and mode of a mounted
       document, attached to each node in the tree.
       """
    def __init__(self, file, mode, attributes, dom):
        self.file = file
        self.mode = mode
        self.attributes = attributes
        self.dom = dom

    def __str__(self):
        # This makes deciphering the mounts array of the Tree much easier
        try:
            name = '"%s"' % self.attributes['title'].value
        except:
            name = str(self.file)
        try:
            mountSpec = " mounted at %s" % self.attributes['root'].value
        except:
            mountSpec = ""
        return "<%s.%s %s%s>" % (self.__module__, self.__class__.__name__, name, mountSpec)

    def __repr__(self):
        return self.__str__()

class Tree(PGBuild.XMLUtil.Document):
    """Configuration tree- an XML document with a <pgbuild> root
       that supports merging in other documents with a <pgbuild>
       root, then saving changes back to those documents.
       """
    def __init__(self):
        PGBuild.XMLUtil.Document.__init__(self,
                                          """<?xml version="1.0" ?>
                                          <pgbuild title="Merged configuration tree"/>
                                          """)
        self.mounts = []

    def mount(self, file, mode="r"):
        """Mount the given document in the config tree, at the
           location specified in that document's pgbuild/@root.
           The default mode of "r" prevents writing changes back,
           a mode of "w" allows writing back changes.
           """
        dom = PGBuild.XMLUtil.Document(file)
        stripElements(dom)
        dom.normalize()

        selfPgb = self.xpath("/pgbuild")[0]

        # Validate the <pgbuild> tag
        try:
            domPgb = dom.xpath("/pgbuild")[0]
        except IndexError:
            raise PGBuild.Errors.UserError(
                "Trying to mount a config file without a <pgbuild> root")

        # Save the document's attributes and it's original DOM in the
        # MountInfo for later access via mounts[]
        mdoc = MountInfo(file, mode, domPgb.attributes, dom)
        self.mounts.append(mdoc)

        # Recursively tag all objects in the
        # new DOM with their MountInfo
        def rTag(element, mdoc):
            element.mdoc = mdoc
            for child in element.childNodes:
                rTag(child, mdoc)
        rTag(dom, mdoc)

        # Merge any top-level processing instruction nodes regardless
        # of mount points. This, for example, lets our tree painlessly
        # inherit a stylesheet defined in the conf package.
        # Note that this merging rule is different than that for tags-
        # parent is disregarded since we only work at the top level,
        # and different attributes do not imply a tag is unique.
        for newNode in dom.childNodes:
            if newNode.nodeType == newNode.PROCESSING_INSTRUCTION_NODE:
                # Delete any existing instruction with the same name
                for oldNode in self.childNodes:
                    if oldNode.nodeType == newNode.nodeType and oldNode.nodeName == newNode.nodeName:
                        self.removeChild(oldNode)
                # The reason for messing with parentNode is explained below
                # in the main appending code.
                newNode.parentNode = None
                self.insertBefore(newNode, selfPgb)

        # This implements the semantics described in sources.xml for the
        # /pgbuild/@root attribute. If it's not present, the document
        # is mounted at the pgbuild element. Otherwise it's an XPath
        # relative to the pgbuild element.
        try:
            mountPath = domPgb.attributes['root'].value
        except KeyError:
            mountPath = '.'

        # Now we need to resolve the mount path to exactly one element.
        while 1:
            matches = self.xpath(mountPath, selfPgb)

            # Only one match? We're done
            if len(matches) == 1:
                mountElement = matches[0]
                break

            # Multiple matches? Try merging the tree first
            if len(matches) > 1:
                mergeElements(self)
                # Still multiple matches? We can't continue
                if len(self.xpath(mountPath, selfPgb)) > 1:
                    raise PGBuild.Errors.UserError("Ambiguous mount point")

            # No match? Create elements as needed to match the path.
            # This can only be done automatically in the simplest case.
            # If there's any trouble here, we'll just raise an error.
            if len(matches) == 0:
                # We don't even try to process anything but the simplest XPaths here
                if re.search("[^a-zA-Z\-_/]", mountPath):
                    raise PGBuild.Errors.UserError(
                        "Mount point doesn't exist, too complex to automatically create")
                # Make it absolute
                if mountPath[0] != '/':
                    absMountPath = "/pgbuild/" + mountPath
                else:
                    absMountPath = mountPath
                # Walk the path, creating elements if necessary
                n = self
                for tag in absMountPath.split("/")[1:]:
                    try:
                        n = n.getElementsByTagName(tag)[0]
                    except IndexError:
                        newN = self.createElement(tag)
                        n.appendChild(newN)
                        n = newN
                if len(self.xpath(mountPath, selfPgb)) == 0:
                    raise PGBuild.Errors.InternalError("Automatic mount point creation failed")

        # Now that we have the source and destination resolved, we can
        # implement the mount by appending all the children of domPgb
        # to mountElement. Then, let mergeElements() sort out the rest.
        for child in domPgb.childNodes:
            # Clear the parentNode to keep minidom from exploding
            # when it tries to remove the node from the original DOM.
            # Note that it's important for parentNode to be valid in
            # the config tree for areElementsEqual() to be accurate.
            # The original DOM will not be strictly correct, but
            # its semantics will still make sense- elements in the
            # original DOM below the root will have a parentNode
            # pointing to the mount point.
            child.parentNode = None
            mountElement.appendChild(child)
        mergeElements(self)
                    
default = Tree()

### The End ###
