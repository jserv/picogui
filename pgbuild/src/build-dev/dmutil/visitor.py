#  $Id: visitor.py,v 1.1 1999/01/15 20:21:44 dieter Exp $
# Copyright (C) 1998-1999 by Dr. Dieter Maurer <dieter@handshake.de>
# D-66386 St. Ingbert, Eichendorffstr. 23, Germany
#
#			All Rights Reserved
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted,
# provided that the above copyright notice and this permission
# notice appear in all copies, modified copies and in
# supporting documentation.
# 
# Dieter Maurer DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL Dieter Maurer
# BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
# DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
# PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
# TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
"""A simple visitor for a DOM tree.

The DOM tree is visited in depth-first left-to-right order.

For each visited node, the list of *handlers* is checked,
whether they want to handle the node. The first handler
not returning 'None' terminates the search through the handler list.
If all handlers return 'None', all children are visited.

A handler may return:
  'None'  --  it is not interested in processing this node
  '0'     --  process all children
  '1'     --  it has processed the subtree, the result is left in place
  *func*  --  process all children then call *func* on the node
              If *func* returns a node, it replaces the visited node,
	      otherwise, *func* should return 'None'
  *node*  --  replace the visited node with *node*
 
**ATTENTION**: if a derived class uses a method of its own as handler
  (which is quite natural), a circular reference is created.
  Such a Visitor is not garbaged collected.
  As a workaround, you may explicitely call 'destroy'. It brakes
  the circular reference, but makes the visitor unusable, too.
"""

__version__= '0.2'

# This copy of Visitor has been modified to use minidom via PGBuild's
# wrapper, rather than using the full DOM.
from xml.dom.minidom import Node

class Visitor:
  ProcessAttributes= 0
  def __init__(self,*handlers):
    self.handlers= handlers	# ATTENTION: may create circular references
  #
  def destroy(self):
    self.handlers= None	# to break potential circular references
  #
  def visit(self,doc):
    """visit document *doc*."""
    self._doc= doc
    return self._visit(doc)
  #
  def _visit(self,node):
    """visit *node* and its subtree."""
    for h in self.handlers:
      r= h(node)
      if r is None: continue
      if isinstance(r,Node): return self._replaceNode(node,r)
      break
    if r == 1: return
    if self.ProcessAttributes:
      attrs= node.get_attributes()
      if attrs:
	for c in attrs.values(): self._visit(c)
    for c in node.get_childNodes(): self._visit(c)
    if callable(r):
      r= r(node)
      if isinstance(r,Node): return self._replaceNode(node,r)
  #
  def _replaceNode(self,node,newnode):
    """replaces *node* by *newnode*."""
    if node is self._doc: return newnode
    parent= node.parentNode
    parent.replaceChild(newnode,node)

