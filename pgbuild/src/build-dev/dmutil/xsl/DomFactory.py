# Copyright (C) 1999 by Dr. Dieter Maurer <dieter@handshake.de>
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
"""PyDOM factory for XSL expressions."""

# This copy of DomFactory has been modified to use minidom via PGBuild's
# wrapper, rather than using the full DOM.
from PGBuild.XML.dom import minidom

from BaseFactory import BaseFactory, Expr, NodeListExpr, EvalContext
from dmutil.visitor import Visitor
from string import find, join



class _EvalContext(EvalContext):
  """an evaluation context used for document level optimizations."""
  def __init__(self,env,parsecontext):
    self._domopt= _DomOpt()
    EvalContext.__init__(self,env,parsecontext)


class _DomOpt:
  """DOM optimization class -- currently used for document ordering, id-references and key-references.

  ATTENTION: assumes, that the source document is **NOT** modified during
    expression evaluation!

  In order for id-references to work, the document must
  have methods 'getIdMap(self)' and 'hasId(self,node,idval)'.
  Otherwise, it is assumed that the document does not possess
  ID attributes. The factory 'IdDecl' provides a document
  with these methods.
  """
  _orders= None
  _idrefs= None
  _keyrefs= None
  #
  def dfs(self,node):
    """returns a pair *(doc,dfs_number)*."""
    doc= node.ownerDocument or node; docid= doc
    if self._orders is None: self._orders= {}
    try: dfs= self._orders[docid]
    except KeyError:
      self._orders[docid]= dfs= dfsNumberer(doc)
    return (docid,dfs[node])
  #
  # id-references
  def hasId(self,node,idval):
    """returns 1 if *node* has ID-attribute with value *idval*."""
    doc= node.ownerDocument
    if doc is None: return 0
    try: hasId= getattr(doc,'hasId')
    except AttributeError: return 0
    return hasId(node,idval)
  #
  def getIdMap(self,node):
    doc= node.ownerDocument or node; docid= doc
    if self._idrefs is None: self._idrefs= {}
    try: return self._idrefs[docid]
    except KeyError:
      try: IdMap= docid._idMap
      except AttributeError: return {}
      self._idrefs[docid]= idmap= _idMapper.visit(doc,IdMap)
      return idmap
    

		

class DomFactory(BaseFactory):
  # extended evaluation contexts
  buildEvalContext= _EvalContext
  # node tests
  def WildcardName(self,name=None):
    return lambda node,env, name=name: _WildcardCheck(node,name,env)
  def text(self):
    return lambda node,env: node.nodeType == minidom.Node.TEXT_NODE
  def comment(self):
    return lambda node,env: node.nodeType == minidom.Node.COMMENT_NODE
  def pi(self,name=None):
    return lambda node,env: node.nodeType == minidom.Node.PROCESSING_INSTRUCTION_NODE \
	                    and name is None or name == node.nodeName
  #
  # axis functions
  def axisfun_ancestor(self,node,nodetest):
    l= []
    while 1:
      node= node.parentNode	# ATTENTION: wrong for attributes!
      if node is None: return l
      if nodetest(node): l.append(node)
  #
  def axisfun_ancestor_or_self(self,node,nodetest):
    l= []
    while 1:
      if nodetest(node): l.append(node)
      node= node.parentNode	# ATTENTION: wrong for attributes!
      if node is None: return l
  #
  def axisfun_attribute(self,node,nodetest=None):
    attrs= node.attributes
    l= []
    if attrs is None: return l
    for a in attrs.values():
      if nodetest is None or nodetest(a):
	a.parentNode= node
	l.append(a)
    return l
  #
  def axisfun_child(self,node,nodetest=None):
    children= node.childNodes
    l= []
    if children is None: return l
    for c in children:
      if nodetest is None or nodetest(c): l.append(c)
    return l
  #
  def axisfun_descendant(self,node,nodetest):
    return _descender.visit(node,nodetest,0)
  #
  def axisfun_descendant_or_self(self,node,nodetest):
    return _descender.visit(node,nodetest,1)
  #
  def axisfun_following(self,node,nodetest):
    l= []
    while 1:
      l.append(node)
      node= node.parentNode
      if node is None: break
    l.reverse()
    res= []; _following(l,nodetest,res)
    return res
  #
  def axisfun_following_sibling(self,node,nodetest):
    l= []
    while 1:
      node= node.nextSibling
      if node is None: return l
      if nodetest(node): l.append(node)
  #
  def axisfun_preceding(self,node,nodetest):
    l= []
    while 1:
      l.append(node)
      node= node.parentNode
      if node is None: break
    l.reverse()
    res= []; _preceding(l,nodetest,res)
    res.reverse()
    return res
  #
  def axisfun_preceding_sibling(self,node,nodetest):
    l= []
    while 1:
      node= node.previousSibling
      if node is None: return l
      if nodetest(node): l.append(node)
  #
  def axisfun_parent(self,node,nodetest):
    node= node.parentNode
    return node and nodetest(node) and [node] or []
  #
  def axisfun_self(self,node,nodetest):
    return nodetest(node) and [node] or []
  #
  # OwnerDocument
  def getOwnerDocument(self,node):
    return node.ownerDocument or node
  #
  # normalize node
  def normNode(self,node): return node
  #
  # nodeQname
  def nodeQName(self,node):
    if node.nodeType in (minidom.Node.ELEMENT_NODE, minidom.Node.ATTRIBUTE_NODE):
      return node.nodeName
    return ''
  #
  def getNodeValue(self,node):
    v= node.nodeValue
    if v is not None: return v
    return _valueVisitor.visit(node)
  #
  def sortDocumentOrder(self,nodeseqlist,env):
    if not nodeseqlist: return []
    if len(nodeseqlist) == 1: return nodeseqlist[0]
    return _sortDocumentOrder(nodeseqlist,env._domopt.dfs)
  #
  def minDocumentOrder(self,nodeseqlist,env):
    if not nodeseqlist: return []
    if len(nodeseqlist) == 1: return nodeseqlist[0]
    dfs= env._domopt.dfs
    m= nodeseqlist[0][0]; mv= dfs(m)
    for nl in nodeseqlist[1:]:
      n= nl[0]; nv= dfs(n)
      if nv < mv: m= n; mv= nv
    return [m]
  #
  # ID reference handling
  def getNodesWithId(self,node,ids,env):
    """returns the list of nodes (in document order) in the document of *node*
    having an ID attribute with value in *ids*.

    ATTENTION: there is room for optimization!
    """
    l= []; idmap= env._domopt.getIdMap(node)
    if not idmap: return l
    for id in ids:
      try: l.append([idmap[id]])
      except KeyError: pass
    return self.sortDocumentOrder(l,env)
  #
  def hasId(self,node,id,env): return env._domopt.hasId(node,id)
  #
  def getAncestorAttribute(self,node,attr):
    """returns the value of *attr* of the first Ancestor (or self)."""
    while 1:
      v= node.getAttribute(attr)
      if v: return v
      node= node.parentNode
      if node is None: return None




def _WildcardCheck(node,name,env):
  if node.nodeType not in (minidom.Node.ELEMENT_NODE, minidom.Node.ATTRIBUTE_NODE): return 0
  if name is None: return 1
  n= node.nodeName
  i= find(n,':')
  if i < 0: ns= ''; ln= n
  else: ns= n[:i]; ln= n[i+1:]
  if name[1] is None: return ns == name[0]
  return ns == name[0] and ln == name[1]




class _DfsNumberer(Visitor):
  ProcessAttributes= 1
  def __init__(self):
    Visitor.__init__(self,self._numberer)
  def visit(self,doc):
    """dictionary mapping nodes to dfs number."""
    self._number= 0
    self._dfs= {}
    self._visit(doc)
    return self._dfs
  def _numberer(self,node):
    self._dfs[node]= self._number
    self._number= self._number + 1

dfsNumberer= _DfsNumberer().visit

def _sortDocumentOrder(nodeseqlist,dfs):
  """merge to nodesequences into a single nodesequence, ordered in document order.

  *nodeseqlist* is a list of nonempty sequences sorted in document order
  with at least 2 elements.
  """
  doc= nodeseqlist[0][0]
  doc= doc.ownerDocument or doc
  n= len(nodeseqlist); wl= [None] * n; i= 0
  for ns in nodeseqlist:
    node= ns[0]; wl[i]= (dfs(node),ns,node); i= i+1
  r= []
  while n > 1:
    wl.sort(); newsort= 0
    v1= wl[1][0]
    while 1:
      (v0,nl0,n0)= wl[0]
      if v0 > v1: break
      r.append(n0)
      if v0 == v1:
	newsort= 1
	j= 1
	while j < n:
	  wlj= wl[j]
	  if v0 == wlj[0]:
	    nl= wlj[1]
	    if len(nl) == 1:
	      n= n-1; j= j-1
	      del wl[j]
	    else:
	      del nl[0]
	      node= nl[0]
	      wl[j]= (dfs(node),nl,node)
	  else: break
	  j= j+1
      if len(nl0) == 1:
	n= n-1; del wl[0]
	break
      del nl0[0]
      node= nl0[0]
      wl[0]= (dfs(node),nl0,node)
      if newsort: break
  return r + wl[0][1]


def _preceding(limits,test,res):
  if len(limits) <= 1: return
  s= limits[0]; del limits[0]; lid= limits[0]
  for n in s.childNodes:
    if n is lid:
      _preceding(limits,test,res)
      return
    _nodeVisitor.visit(n,test,res)

def _following(limits,test,res):
  if len(limits) <= 1: return
  del limits[0]; l= limits[0]
  _following(limits,test,res)
  while 1:
    l= l.nextSibling
    if l is None:
      return
    _nodeVisitor.visit(l,test,res)


class _Descender(Visitor):
  def __init__(self):
    Visitor.__init__(self,self._descend)
  def visit(self,doc,test,accept_root=1):
    selfs= []
    self._root= doc
    self._test= test
    self._accept_root= accept_root
    self._visit(doc)
    return selfs
  def _descend(self,node):
    nnode= node
    if (nnode != self._root or self._accept_root) and self._test(node):
      selfs.append(node)

_descender= _Descender()



class _NodeVisitor(Visitor):
  def __init__(self):
    Visitor.__init__(self,self._descend)
  def visit(self,doc,test,res):
    self._res= res
    self._test= test
    self._visit(doc)
    return self._res
  def _descend(self,node):
    if self._test(node): self._res.append(node)

_nodeVisitor= _NodeVisitor()



class _ValueVisitor(Visitor):
  def __init__(self):
    Visitor.__init__(self,self._classify)
  #
  def visit(self,node):
    """the value of *node*."""
    self.valfragments=[]
    self._visit(node)
    return join(self.valfragments,'')
  #
  def _classify(self,node):
    nt= node.nodeType
    if nt in (minidom.Node.ELEMENT_NODE, minidom.Node.DOCUMENT_NODE): return None
    if nt == minidom.Node.TEXT_NODE: self.valfragments.append(node.nodeValue)
    return 1

_valueVisitor= _ValueVisitor()



##########################################################################
## Id references
class IdDecl:
  """adding IdMapping information to a document."""
  def __init__(self,idmap):
    """*idmap* must be a dictionary mapping element types
    to attribute names. *idmap[e]=a* specifies that *a* is
    the id attribute for element type *e*.
    """
    self.idMap= idmap
  #
  def __call__(self,document):
    document._document._idMap= self.idMap


class _IdMapper(Visitor):
  def __init__(self): Visitor.__init__(self,self._check)
  #
  def visit(self,doc,idmap):
    self._idmap= idmap
    self._dict= {}
    self._visit(doc)
    return self._dict
  #
  def _check(self,node):
    if node.nodeType == minidom.Node.ELEMENT_NODE:
      idmap= self._idmap; et= node.nodeName
      if idmap.has_key(et):
        a= idmap[et]
	id= node.getAttribute(a)
	d= self._dict
	if d.has_key(id):
	  raise error,'id %s not unique' % id
	d[id]= node

_idMapper= _IdMapper()
