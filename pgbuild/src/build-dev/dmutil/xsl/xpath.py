# $Id: xpath.py,v 1.1 1999/08/03 21:05:57 dieter Exp dieter $
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
"""PyXPATH

PyXPATH is an implementation of the XML Path Language (XPath),
Version 1.0 as of
working draft 9-July-1999:
	URL:http://www.w3.org/1999/07/WD-xslt-19990709

XPath is a core part of both XPointer and XSLT, the XSL transformation
language. They are used for computations, tests, string
manipulation and tree operations such as selecting nodes
and matching nodes against patterns.
They can be used outside XSL, too, for e.g. querying/selecting
parts of HTML/SGML/XML documents.

PyXPATH is laid on top of PyDom, the DOM implementation
of Pythons XML special interest group:
	URL:http://www.python.org/sigs/xml-sig/files/xml-0.5.1.tgz
	URL:http://www.python.org/sigs/xml-sig/files/xml-0.5.1.zip

There are some deviations from the above XPath specification:

 * XPath requires namespace handling. This is not (yet) observed,
   because PyDom does not yet support namespaces.
   More precisely: the current implementation does not
   resolve namespace prefixes to URI's but performs
   comparisons with the prefixes rather than the URI's.
   
 * 'id' references require the document to contain a method
   'getIdMap' returning a dictionary mapping id names to nodes.

   Such documents can be created with 'IdDecl' factories.
   They are constructed from an 'id' list and an optional
   element-id map, specifying which attributes are used
   as ids. Applied to a document, they add to it the
   required method.
   
 * CDATA sections and entity references are not yet handled correctly.
   Most parsers resolve CDATA sections and entity references
   into normal text nodes (if possible and not told otherwise).
   In the resulting DOM tree, there may be adjacent text nodes
   which is forbidden by the XSL specification.
   The function 'normalize' merges adjacent text nodes
   if applied to document.

 * The current version works with the ISO Latin 1 subset
   of Unicode rather than Unicode itself, violating the XML
   specification.
   The reason is that Python does not yet have full Unicode support.

 * The implementation raises exceptions when number operations would
   involve special floating point values (NaN, positive and negative
   infinity).
   This is because Python does not support such values.
"""

__version__= '0.1'

from PyBison.bisonparser import BisonParser
import xpathparser
import re
from string import upper, replace, atof, find


##############################################################################
## XML whitespace
_WHITESPACE=r'[\x20\x9\xd\xa]'
_re_spaces= re.compile(_WHITESPACE+'+')


##############################################################################
## XML character classes stripped down to 8 bit
# Letter
_LETTER= '[\x41-\x5a\x61-\x7a\xc0-\xd6\xd8-\xf6\xf8-\xff]'
# XML NCNameChar (Namespace Constraint Name Char)
_NCNAMECHAR= '(?:'+_LETTER+'|[-\\d._\xb7])'
# XML NCName (Namespace Contraint Name)
_NCNAME= '(?:(?:'+_LETTER+'|_)'+_NCNAMECHAR+'*)'
_QNAME= '(?:%s:)?%s' % (_NCNAME,_NCNAME)


##############################################################################
## Error class


class Error(Exception):
  """General Errors"""

class SyntaxError(Error):
  """Syntax Errors"""




##############################################################################
## XPathParser

def _dictify(names,factory,token=None):
  """dictionary with *names* as keys and corresponding (token,impl) pairs as value."""
  d= {}
  for name in names:
    pyname=  replace(name,'-','_')
    tok= token
    if tok is None: tok= getattr(xpathparser,upper(pyname))
    try: impl= getattr(factory,pyname)
    except AttributeError:
      try: impl= getattr(factory,'Imp_' + pyname)
      except AttributeError: impl= factory.Unimplemented(name)
    d[name]= (tok,impl)
  return d


class XPathParser(BisonParser):
  """Parser based on PyBisons 'BisonParser'.

  PyBison is a Python backend to the 'bison' parser
  generator. PyBison was developped by Scott Hassan
  URL:mailto://hassan@cs.stanford.edu.
  It (probably) can be found at:
  URL:http://www-db.stanford.edu/~hassan/Python/pybison.tar.gz.
  """
  def __init__(self,context=None,factory=None):
    if factory is None:
      import DomFactory
      factory= DomFactory.DomFactory()
    self._parsecontext= pc= _ParseContext(context,factory)
    BisonParser.__init__(self,xpathparser,pc)
    self.factory= factory
    self._Operators= _dictify(self._opnames,factory)
    self._Axis= _dictify(self._axisnames,factory,xpathparser.AXISNAME)
    self._NodeTypes= _dictify(self._nodetypenames,factory,xpathparser.NODETYPE)
    self._is_OperatorName= self._Operators.has_key
    self._is_AxisName= self._Axis.has_key
    self._is_NodeTypeName= self._NodeTypes.has_key
  #
  def parse(self,patternstring,context=None):
    """parses *patternstring* into an XPath."""
    self._init_tokenize(patternstring)
    self._parsecontext.extend(context)
    p= self.yyparse(None)
    p.patternstring= patternstring
    return p
  #
  __call__= parse
  #
  # Tokenizer
  _match_space= re.compile(_WHITESPACE+'*').match
  _match_followsParen= re.compile(_WHITESPACE+r'*\(').match
  _match_followsAxisSuf= re.compile(_WHITESPACE+'*::').match
  _match_Literal= re.compile('\'[^\']*\'|\"[^\"]*\"').match
  _match_Number= re.compile(r'[0-9]+(\.[0-9]+)?|\.[0-9]+').match
  _match_Varref= re.compile(r'\$(%s)' % _QNAME).match
  _match_QName= re.compile(_QNAME).match
  _match_Wildcard= re.compile(r'(%s):\*' % _NCNAME).match
  _seps= '!/@()|[]=.*,<>+-'
  _opnames= ('and', 'or', 'mod', 'div', 'quo')
  _opinhibitor= '@([,/|+-=<>'
  _axisnames= ('ancestor', 'ancestor-or-self',
	      'attribute', 'child',
	      'descendant', 'descendant-or-self',
	      'following', 'following-sibling',
	      'parent',
	      'preceding',  'preceding-sibling',
	      'self')
  _nodetypenames= ('comment', 'text', 'processing-instruction', 'node')
  #
  def _init_tokenize(self,patternstring):
    self._string= patternstring
    self._strlen= len(patternstring)
    self._strpos= 0
    self._inhibit_op= 1
  #
  def _skip_blank(self):
    m= self._match_space(self._string,self._strpos)
    if m: self._strpos= m.end()
  #
  def tokenize(self):
    self._skip_blank()
    p= self._strpos; n= self._strlen; s= self._string; tok= None
    if p >= n: return ()
    m= self._match_Number(s,p)
    if m:
      # a number
      tok=(xpathparser.NUMBER,atof(m.group(0)))
      p= m.end()
      self._inhibit_op= 0
    else:
      c= s[p]
      if c == ':' and p < n-1 and s[p+1] == ':':
	self._inhibit_op= 1
	p= p+2
	tok= xpathparser.AXISSUF
      elif c in self._seps:
	p= p+1
	if c == '.' and p < n and s[p] == '.':
	  self._inhibit_op= 0
	  p= p+1
	  tok= xpathparser.PARENT
	elif c == '<' and p < n and s[p] == '=':
	  self._inhibit_op= 1
	  p= p+1
	  tok= xpathparser.LE
	elif c == '>' and p < n and s[p] == '=':
	  self._inhibit_op= 1
	  p= p+1
	  tok= xpathparser.GE
	elif c == '!' and p < n and s[p] == '=':
	  self._inhibit_op= 1
	  p= p+1
	  tok= xpathparser.NE
	elif c == '*':
	  if self._inhibit_op:
	    tok= (xpathparser.WILDCARDNAME,self.factory.WildcardName())
	    self._inhibit_op= 0
	  else:
	    tok= (xpathparser.MULTIPLYOPERATOR,self.factory.mult)
	    self._inhibit_op= 1
	else:
	  self._inhibit_op= c in self._opinhibitor
	  tok= ord(c)
      elif c == '$':
	# a variable reference
	m= self._match_Varref(s,p)
	if not m: self._syntax_error('unrecognized token')
	p= m.end()
	tok= (xpathparser.VARIABLEREFERENCE,
	      self.factory.VariableReference(m.group(1)))
      else:
	# it must be a name or literal
	m= self._match_QName(s,p)
	if m:
	  # a name: may be a WildcardName, a NodeType, an Operator
	  #                an AxisIdentifier or a function name
	  name= m.group(); p= m.end()
	  if not self._inhibit_op and self._is_OperatorName(name):
	    tok= self._Operators[name]
	    self._inhibit_op= 1
	  else:
	    self._inhibit_op= 0
	    if self._match_followsParen(s,p):
	      if self._is_NodeTypeName(name): tok= self._NodeTypes[name]
	      else: tok= (xpathparser.FUNCTIONNAME,self._parsecontext._resolveFunction(name))
	    elif self._is_AxisName(name) and self._match_followsAxisSuf(s,p):
	      tok= self._Axis[name]
	    else:
	      # a QName as WildcardName
	      self._inhibit_op= 0
	      i= find(name,':')
	      if i < 0: pref= ''; locname= name
	      else: pref= name[:i]; locname= name[i+1:]
	      tok= (xpathparser.WILDCARDNAME,
		    self.factory.WildcardName((self._parsecontext._resolveNamespace(pref),locname)))
	else:
	  self._inhibit_op= 0
	  m= self._match_Wildcard(s,p)
	  if m:
	    pref= m.group(1)
	    p= m.end()
	    tok= (xpathparser.WILDCARDNAME,
		  self.factory.WildcardName((self._parsecontext._resolveNamespace(pref),None)))
	  else:
	    m= self._match_Literal(s,p)
	    if not m: self._syntax_error('unrecognized token')
	    p= m.end()
	    tok= (xpathparser.LITERAL,m.group()[1:-1])
    self._strpos= p
    return (tok,)
  #
  def _syntax_error(self,text):
    raise SyntaxError(text,self._string,self._strpos)
  #
  def yyerrlab1(self):
    self._syntax_error('invalid expression syntax')
  #
 
class _ParseContext:
  def __init__(self,context,factory):
    self._basecontext= context; self._factory= factory
  #
  def __getattr__(self,attr): return getattr(self._factory,attr)
  #
  def extend(self,context):
    self._context= context
  #
  def _resolveNamespace(self,pref):
    if not pref: return ''
    return _lookupNamespace(self._context,pref) or _lookupNamespace(self._basecontext,pref) or pref + ':'
  #
  def _resolveFunction(self,name):
    i= find(name,':')
    if i < 0: pref= ''
    else: pref= name[:i]; name= name[i+1:]
    pref= self._resolveNamespace(pref)
    return _lookupFunction(self._context,pref,name) \
	   or _lookupFunction(self._basecontext,pref,name) \
	   or self._factory._lookupFunction(pref,name) \
	   or self._factory.Unimplemented(pref,name)

def _lookupNamespace(context,prefix):
  return context and context.getNamespace(prefix)

def _lookupFunction(context,prefix,name):
  return context and context.getFunction(prefix,name)
  


class ParseContext:
  """parse context:
     contains: namespace definitions
               function definitions."""
  def __init__(self):
    self._namespaces= {}; self._functions= {}
  #
  def setNamespace(self,prefix,val): self._namespaces[prefix]= val
  def getNamespace(self,prefix): return self._namespaces.get(prefix)
  def delNamespace(self,prefix):
    try: del self._namespaces[name]
    except KeyError: pass
  #
  def setFunction(self,name,val): self._functions[name]= val
  def getFunction(self,name): return self._functions.get(name)
  def delFunction(self,name):
    try: del self._functions[name]
    except KeyError: pass


class Env:
  """evaluation environment:
     contains: variable bindings."""
  def __init__(self):
    self._variables= {}
  #
  # variable binding handling
  def setVariable(self,name,val): self._variables[name]= val
  def getVariable(self,name): return self._variables.get(name)
  def getVariableNames(self): return self._variables.keys()
  def delVariable(self,name):
    try: del self._variables[name]
    except KeyError: pass


makeParser= XPathParser
