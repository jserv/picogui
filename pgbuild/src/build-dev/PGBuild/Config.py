""" PGBuild.Config

Implements PGBuild's configuration tree, formed from multiple XML
documents with some extra rules and a 'mount point' system.

The configuration tree is an in-memory XML document, subclassed
from PGBuild.XMLUtil.Document. This document is populated by
'mounting' XML files. A read-only mount will have no further
effect after its contents are read and merged with the config
document. A read-write mount will allow later saving modifications
to mounted elements back to the original XML document.

The extra rules imposed on the XML:

1. Any elements with the same parent, name, and attributes, are
   treated as equal. They will be merged- child elements are appended,
   child text nodes are replaced.

2. In merging child nodes, order is preserved

3. Python attributes from the second of the two nodes are preserved
   when merging. This means that metadata like the 'minfo' attribute
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

def prependElements(src, dest):
    """Move all children from the 'src' into 'dest'.
       Elements get prepended, text nodes only get
       moved if dest has no text.
       After exectuing this, src has no children.
       """
    destHasText = 0
    for child in dest.childNodes:
        if child.nodeType == child.TEXT_NODE:
            destHasText = 1
            break
    while src.childNodes:
        last = src.lastChild
        if last.nodeType == last.TEXT_NODE and destHasText:
            # Discard this text node, the dest already has text
            src.removeChild(last)
        else:
            if dest.childNodes:
                dest.insertBefore(last, dest.childNodes[0])
            else:
                dest.appendChild(last)

def appendElements(src, dest):
    """Move all children from the 'src' into 'dest'.
       Elements are appended, all text nodes in dest are
       overwritten if src has any text.
       After exectuing this, src has no children.
       """
    srcHasText = 0
    for child in src.childNodes:
        if child.nodeType == child.TEXT_NODE:
            srcHasText = 1
            break
    if srcHasText:
        # Copy the list, since we'll be modifying it
        for child in dest.childNodes[:]:
            if child.nodeType == child.TEXT_NODE:
                dest.removeChild(child)
    for child in src.childNodes[:]:
        dest.appendChild(child)

def mergeElements(root):
    """Merge all identical elements under the given one,
       as defined in this module's document string.
       """
    # Make a dictionary of signatures to (relatively) efficiently
    # determine whether any of our children are duplicates.
    d = {}
    # Duplicate the child list here, so when it's modified below,
    # our iteration doesn't go all wonky.
    for child in root.childNodes[:]:
        sig = repr([child.nodeType, child.nodeName, PGBuild.XMLUtil.getAttrDict(child)])
        if d.has_key(sig):
            # This is a duplicate! Prepend all the children from
            # the first one, and delete it.
            old = d[sig]
            prependElements(old, child)
            old.parentNode.removeChild(old)
        d[sig] = child
    del d

    # After all the merging at this level, traverse depth-first
    for child in root.childNodes:
        mergeElements(child)

def stripElements(root):
    """Recursively trim whitespace from text elements.
       We have to be careful not to nuke whitespace-only text
       nodes that are separating two other text elements.
       """
    for child in root.childNodes:
        if child.nodeType == child.TEXT_NODE:
            child.data = child.data.strip()
            if len(child.data) == 0 and child.previousSibling and \
                   child.previousSibling.nodeType == child.TEXT_NODE and \
                   child.nextSibling and child.nextSibling.nodeType == child.TEXT_NODE:
                child.previousSibling.data += " " + child.nextSibling.data.strip()
                child.nextSibling.data = ""
        else:
            stripElements(child)

class MountInfo:
    """Object defining the file and mode of a mounted
       document, attached to each node in the tree.
       """
    def __init__(self, file, mode, attributes, dom):
        self.file = file
        self.mode = mode
        self.attributes = attributes
        self.dom = None

        if mode != 'r' and mode != 'rw':
            raise PGBuild.Errors.InternalError("Unknown mount mode")

        # Tree will end up destroying the original DOM as it moves nodes out
        # of it for the merge process. Normally this is fine, but we'll need
        # to save a pristine deep copy for any read-write mounts so we can edit
        # and save them.
        if mode == 'rw':
            self.dom = dom.cloneNode(1)
            # At this point it's easy to point our original to the pristine copy,
            # so let's do so and save us much headache when it comes time to call
            # Tree.commit().
            def link(copy, original):
                original.minfoNode = copy
                for i in xrange(len(original.childNodes)):
                    link(copy.childNodes[i], original.childNodes[i])
            link(self.dom, dom)

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
        return "<%s.%s %s from %s%s with mode '%s'>" % (
            self.__module__, self.__class__.__name__, name, self.file, mountSpec, self.mode)

    def __repr__(self):
        return self.__str__()

class Tree(PGBuild.XMLUtil.Document):
    """Configuration tree- an XML document that supports merging
       in other documents with the same root type, then saving
       changes back to those documents.
       """
    def __init__(self, title="Merged configuration tree", rootName="pgbuild"):
        PGBuild.XMLUtil.Document.__init__(
            self, '<?xml version="1.0" ?><%s title="%s"/>' % (rootName, title))
        self.rootName = rootName
        self.title = title
        self.mounts = []

    def mount(self, file, mode="r"):
        """Mount the given document in the config tree, at the
           location specified in that document's pgbuild/@root.
           The default mode of "r" prevents writing changes back,
           a mode of "w" allows writing back changes.
           """
        dom = PGBuild.XMLUtil.Document(file)

        # Validate the <pgbuild> tag
        if dom.getRoot().nodeName != self.rootName:
            raise PGBuild.Errors.UserError(
                "Trying to mount a config tree with a <%s> root where <%s> is expected" %
                (dom.getRoot().nodeName,self.rootName))

        # Recursively tag all objects in the
        # new DOM with their MountInfo
        def rTag(element, minfo):
            # minfo is a reference to the MountInfo() object for this node's mount.
            # minfoNode is a reference to the original DOM node corresponding to a
            #   mounted node, only set for writeable mounts. It is assigned in
            #   MountInfo() after it has made a copy of the original DOM.
            element.minfo = minfo
            element.minfoNode = None
            for child in element.childNodes:
                rTag(child, minfo)
        rTag(dom, minfo)

        # Save the document's attributes and it's original DOM in the
        # MountInfo for later access via mounts[]
        minfo = MountInfo(file, mode, dom.getRoot().attributes, dom)
        self.mounts.append(minfo)

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
                self.insertBefore(newNode, self.getRoot())

        # This implements the semantics described in sources.xml for the
        # /pgbuild/@root attribute. If it's not present, the document
        # is mounted at the pgbuild element. Otherwise it's an XPath
        # relative to the pgbuild element.
        try:
            mountPath = dom.getRoot().attributes['root'].value
        except KeyError:
            mountPath = '.'

        # Now we need to resolve the mount path to exactly one element.
        while 1:
            matches = self.xpath(mountPath)

            # Only one match? We're done
            if len(matches) == 1:
                mountElement = matches[0]
                break

            # Multiple matches? Try merging the tree first
            if len(matches) > 1:
                mergeElements(self)
                # Still multiple matches? We can't continue
                if len(self.xpath(mountPath)) > 1:
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
                    absMountPath = "/%s/%s" % (self.rootName, mountPath)
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
                if len(self.xpath(mountPath)) == 0:
                    raise PGBuild.Errors.InternalError("Automatic mount point creation failed")

        # Now that we have the source and destination resolved, we can
        # implement the mount by appending all the children of dom.getRoot()
        # to mountElement. Then, let mergeElements() sort out the rest.
        appendElements(dom.getRoot(), mountElement)

        # Strip whitespace, strip empty text nodes, and merge the tree
        stripElements(self)
        self.normalize()
        mergeElements(self)

    def commit(self):
        """Save all changes to the config tree. This involves first propagating
           changes from the config tree into the pristine DOMs, then saving those DOMs.
           Note that changes to areas of the tree without read-write mounts are ignored,
           since this doesn't yet have any way to find out whether a change has been made-
           read-write mounts always get written to when this is called.
           """

        def propagate(node):
            # Note: this doesn't yet support deleting nodes out of the originals
            #       that have been deleted in the mounted copy.

            # Do we have a place to store this node?
            if node.minfoNode:
                # Store attributes and data
                if node.nodeType == node.ELEMENT_NODE:
                    node.minfoNode.attributes = node.attributes
                if node.nodeType == node.TEXT_NODE:
                    node.minfoNode.data = node.data
                    
            for child in node.childNodes:
                if not child.minfoNode and node.minfoNode:
                    # This is an 
                    child.minfoNode = node.minfoNode
                self.

                # 
                

        # Start the propagation at the document root. Note that this means nodes like
        # XML processing instructions that are outside the root node won't be
        # automatically saved.
        propagate(self.getRoot())
                    
default = Tree()

### The End ###
