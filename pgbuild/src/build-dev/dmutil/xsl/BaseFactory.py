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
"""BaseFactory

 (incomplete) base factory to build XPath-Expr's.
 It contains, what is not dependant on trees.
"""

from types import StringType, FloatType, IntType, ListType, TupleType, \
     		  InstanceType
from string import atof, find, index, join
import re, sys, string, os
from dmutil.classes import bases

XSLVERSION= '1.0'
XSLURI= 'http://www.w3.org/xsl/transform/'


##############################################################################
# value classifications
UNKNOWN=	0
BOOLEAN=	1
NUMBER=		2
STRING=		3
NODESET=	4
TREEFRAGMENT=	5

def classify(val):
  """classify type of *val*."""
  t= type(val)
  if t in IntType: return BOOLEAN
  elif t in FloatType: return NUMBER
  elif t is StringType: return STRING
  elif t in [TupleType, ListType]: return NODESET
  elif t in InstanceType: return TREEFRAGMENT
  else: return UNKNOWN


##############################################################################
# context requirements

NO_CONTEXT=	0
ENV_CONTEXT=	1
NODE=		2
NODEPOSITION=	3
NODELIST=	4

EXPR=		-1	# special context value indicating NO_CONTEXT,
			# but the expression
			# maybe, we should avoid this!



##############################################################################
## XML whitespace
_WHITESPACE=r'[ \t\r\n]'     # <-- This whitespace line patched like the one in xpath.py --Micah
_re_spaces=re.compile(_WHITESPACE+'+')
_re_strip_spaces= re.compile('%s*(.*?)%s*$' % (_WHITESPACE,_WHITESPACE),re.S)


##############################################################################
# optimized axis values
#  document ordered axis have positive numbers
#  optimizable axis are betwenn "FROM_CHILDREN" and "FROM_SELF"
FROM_CHILDREN=			1
FROM_DESCENDANTS=		2
FROM_DESCENDANTS_OR_SELF=	3
FROM_ATTRIBUTES=		4
FROM_SELF=			5
FROM_FOLLOWING=			6
FROM_FOLLOWING_SIBLINGS=	7
FROM_OTHER=			0



##############################################################################
# Expr

class Expr:
  """XSLT-Expr base class."""
  def __init__(self,type,needed_context,factory):
    self.type= type
    self.needed_context= needed_context
    self.factory= factory
  #
  # Evaluations
  def evalForType(self,type,node,nodelist,env):
    v= self.eval(node,nodelist,env)
    if type == self.type or type == UNKNOWN: return v
    else: return self.convertToType(type,v)
  #
  def eval(self,node,nodelist,env):
    raise NotImplementedError,"Expr.eval is abstract"
  #
  # Conversions
  def convertToType(self,type,val):
    return self._convDict[type](self,val)
  def convertToBoolean(self,val): return not not val
  #
  def convertToNumber(self,val):
    t= type(val)
    if t is FloatType: return val
    if t is IntType: return float(val)
    if t is not StringType: val= self.convertToString(val)
    return atof(val)
  #
  def convertToString(self,val):
    t= type(val)
    if t is StringType: return val
    if t is IntType: return val and 'true' or 'false'
    if t is FloatType:
      if val == int(val): return `int(val)`
      else: return `val`
    if t in [ListType, TupleType]:
      return val and self.factory.getNodeValue(val[0]) or ''
    elif t is InstanceType: return self.factory.getNodeValue(val)
    else: raise TypeError, "convertToString: value type not recognized: " + str(t)
  #
  def convertToNodeList(self,val):
    raise TypeError, 'cannot convert to NodeList'
  #
  _convDict= {BOOLEAN: convertToBoolean,
	      NUMBER: convertToNumber,
	      STRING: convertToString,
	      NODELIST: convertToNodeList,
	      }

  
class FunCallExpr(Expr):
  """Basic Function Call expression.

  Such an expression evaluates the function and each argument
  in the environment and then applies the evaluated function
  on the evaluated arguments.

  If the function is a constant ('needed_context == NO_CONTEXT',
  it is not evaluated in the environment,
  if it is only dependent on 'env' ('needed_context == ENV_CONTEXT'),
  it is applied only to 'env'.
  """
  def __init__(self,fundesc,args,factory):
    (impl,restype,argtypes,needed_context)= fundesc
    self.argno= len(args)
    self.argtypes= checkArgtypes(argtypes,args)
    self.args= args
    self.impl= impl
    self.fun_context= needed_context
    if needed_context == EXPR: needed_context= NO_CONTEXT
    if args:
      needed_context= max(needed_context,
			  max(map( lambda a: a.needed_context, args)))
    Expr.__init__(self, restype, needed_context, factory)
  #
  def eval(self,node,nodelist,env):
    n= self.argno; args= self.args; argtypes= self.argtypes;
    eargs= n * [None]; i= 0
    while i < n:
      eargs[i]= args[i].evalForType(argtypes[i],node,nodelist,env)
      i= i+1
    needed_context= self.fun_context
    if needed_context == NO_CONTEXT: impl= self.impl
    elif needed_context == ENV_CONTEXT: impl= self.impl(env)
    elif needed_context == EXPR: impl= self.impl(self)
    else: impl= self.impl(node,nodelist,env,self)
    return apply(impl,eargs)



def round_fun(s):
  i= int(s)
  if i == s: return s
  if i < 0: i= i-1
  d= s-i
  if d == 0.5:
    if i & 1: return float(i+1)
    else: return float(i)
  if d < 0.5: return float(i)
  else: return float(i+1)

def substring_after_fun(s,c):
  i= find(s,c)
  if i == -1: return ''
  return s[i+len(c):]

def normalize_fun(s):
  s=_re_strip_spaces.match(s).group(1)
  return _re_spaces.sub(lambda m:' ',s)

def system_property_fun(s,env):
  i= find(s,':')
  try:
    if i >= 0:
      ns= s[:i]; ln= s[i+1:]
      if hasattr(env,'namespaces'):
        return find(string.lower(env.namespaces[ns]), XSLURI)== 0 \
	       and ln == 'version' and XSLVERSION \
	       or ''
      else: return ns == 'xsl' and ln == 'version' and XSLVERSION or ''
    else: return os.environ[s]
  except KeyError: return ''


class OptCurrentNode:
  args= None
  def __init__(self,node,fun,*args):
    self.node= node; self.fun= fun
    if args: self.args= args
  def __call__(self,*args):
    if args: arg= args[0]
    else: arg= (self.node,)
    if self.args: return apply(self.fun,(arg,) + self.args)
    return self.fun(arg)
  

def name_select(nodeset,type,factory,env=None):
  if not nodeset: return ''
  name= factory.nodeQName(nodeset[0])
  if type == 0: return name
  i= find(name,':')
  if i < 0: ns= ''; ln= name
  else: ns= name[:i]; ln= name[i+1:]
  if type == 1: return ln
  if hasattr(env,'namespaces'): return env.namespaces[ns]
  return ns

def relationalCompare(e,c,o1,o2):
  t1= type(o1); t2= type(o2)
  if t1 in (TupleType, ListType) or t2 in (TupleType, ListType):
    return nodesetCompare(e,c,o1,o2)
  return c(e.convertToNumber(o1), e.convertToNumber(o2))

def equalityCompare(e,c,o1,o2):
  t1= type(o1); t2= type(o2)
  if t1 in (TupleType, ListType) or t2 in (TupleType, ListType):
    return nodesetCompare(e,c,o1,o2)
  if t1 is IntType or t2 is IntType: conv= e.convertToBoolean
  elif t1 is FloatType or t2 is FloatType: conv= e.convertToNumber
  else: conv= e.convertToString
  return c(conv(o1),conv(o2))

def nodesetCompare(e,c,o1,o2):
  """at least one of *o1* or *o2* must be a nodeset."""
  t1= type(o1); t2= type(o2)
  if t1 in (TupleType, ListType) and t2 in (TupleType, ListType):
    cs= e.factory.getNodeValue
    m= len(o2); vgl= m * [None]
    for n in o1:
      j= 0; v= cs(n)
      while j < m:
	w= vgl[j]
	if w is None: w= vgl[j]= cs(o2[j])
	if c(v,w): return 1
	j= j+1
    return 0
  if t1 in (TupleType, ListType):
    if t2 is IntType: return c(e.convertToBoolean(o1),o2)
    if t2 is FloatType: conv= e.convertToNumber
    else: conv= e.convertToString
    for n in o1:
      try:
	if c(conv((n,)),o2): return 1
      except ValueError: continue
    return 0
  if t1 is IntType: return c(o1,e.convertToBoolean(o2))
  if t1 is FloatType: conv= e.convertToNumber
  else: conv= e.convertToString
  for n in o2:
    try:
      if c(o1,conv((n,))): return 1
    except ValueError: continue
  return 0


def idFun(s,n,e,o):
  if type(o) in (TupleType, ListType):
    idlists= map(s.factory.getNodeValue, o)
  else: idlists= (s.convertToString(o),)
  l= []
  for idl in idlists:
    l[len(l):]= filter(None,_re_spaces.split(idl))
  # ATTENTION: optimization potential
  return s.factory.getNodesWithId(n,l,e)


def _reverseString(s):
  l= list(s); l.reverse()
  return join(l,'')

def translate(s,f,t):
  if len(f) > len(t):
    dels= f[len(t):]; f= f[:len(t)]
  else: dels= ""
  if len(f) < len(t): t= t[:len(f)]
  f=_reverseString(f); t=_reverseString(t)
  return string.translate(s,string.maketrans(f,t),dels)

def langFun(lang,langattr):
  if langattr is None: return 0
  return re.match("%s(?:-.*)?$" % lang,langattr,re.I) and 1 or 0

def substring_fun(s,n,l=None):
  n= n-1
  if l is None: e= sys.maxint
  else: e= n+l
  if n < 0: n= 0
  if e < 0: e= 0
  ni= int(n)
  if n != ni: ni= ni+1
  ei= int(e)
  if e != ei: ei= ei+1
  return s[ni:ei]


lambda s, n, l=1000000000: s[int(n):int(n)+int(l)-1],

BaseFunctions= {
  # nodeset functions -- independant of factory
  'last' : (lambda n,nl,e,s: lambda nl=nl: float(len(nl)),
	    NUMBER, (), NODELIST),
  'position': (lambda n,nl,e,s: lambda n=n, nl=nl, norm=s.factory.normNode:
	                           float(map(lambda n,norm=norm: n and norm(n),
					     nl).index(norm(n))+1),
	       NUMBER, (), NODEPOSITION),
  'count' : (lambda ns: float(len(ns)), NUMBER, (NODESET,), NO_CONTEXT),
  'id': (lambda n,nl,e,s: lambda o, n=n, e=e, s=s: idFun(s,n,e,o),
	       NODESET, (UNKNOWN,), NODE),
  'local-part' : (lambda n,nl,e,s: OptCurrentNode(n,name_select,1,s.factory),
		  STRING, ((NODESET,0,1),), NODE),
  'namespace' : (lambda n,nl,e,s: OptCurrentNode(n,name_select,-1,s.factory,e),
		  STRING, ((NODESET,0,1),), NODE),
  'name' : (lambda n,nl,e,s: OptCurrentNode(n,name_select,0,s.factory),
		  STRING, ((NODESET,0,1),), NODE),
  # String functions
  'string' : (lambda n,nl,e,s: OptCurrentNode(n,s.convertToString),
	       STRING, ((STRING,0,1),), NODE),
  'concat' : (lambda *args: apply(string.join,(args,'')),
	        STRING, ((STRING,2,sys.maxint),), NO_CONTEXT),
  'starts-with' : (lambda s,p: s[:len(p)] == p,
		   BOOLEAN, (STRING, STRING), NO_CONTEXT),
  'contains' : (lambda s,c: find(s,c) >= 0,
		   BOOLEAN, (STRING, STRING), NO_CONTEXT),
  'substring-before' : (lambda s,c: s[:max(0,find(s,c))],
			STRING, (STRING, STRING), NO_CONTEXT),
  'substring-after' : (substring_after_fun,
			STRING, (STRING, STRING), NO_CONTEXT),
  'substring' : (substring_fun,
		 STRING, (STRING, (NUMBER,1,2)), NO_CONTEXT),
  'string-length': (lambda s: float(len(s)),
		    NUMBER, (STRING,), NO_CONTEXT),
  'normalize': (lambda n, nl, e, s:
		OptCurrentNode(n, lambda v,s=s: normalize_fun(s.convertToString(v))),
		STRING, ((STRING,0,1),), NODE),
  'translate': (translate,
		STRING, (STRING,STRING,STRING), NO_CONTEXT),
#  'format_number': (lambda e,s: 1/0, # FIXME
#		    STRING, (NUMBER, (STRING,1,2)), ENV_CONTEXT),
  # boolean functions
  'boolean' : (lambda b: b, BOOLEAN, (BOOLEAN,), NO_CONTEXT),
  'not' : (lambda b: not b, BOOLEAN, (BOOLEAN,), NO_CONTEXT),
  'true' : (lambda : 1, BOOLEAN, (), NO_CONTEXT),
  'false' : (lambda : 0, BOOLEAN, (), NO_CONTEXT),
  'lang': (lambda n,nl,e,s: lambda o, n=n, s=s: langFun(o,s.factory.getAncestorAttribute(n,"xml:lang")),
	       BOOLEAN, (STRING,), NODE),
  # number functions
  'number' : (lambda n,nl,e,s: OptCurrentNode(n,s.convertToNumber),
	       NUMBER, ((NUMBER,0,1),), NODE),
  'sum' : (lambda e: lambda ns, e=e: reduce(lambda a,b: a+b,
					    map(e.convertToNumber,
						map(lambda n: (n,),ns)),
					    0),
	   NUMBER, (NODESET,), EXPR),
  'floor' : (lambda s: float( s==int(s) and s or (s<0 and int(s)-1 or int(s))),
	       NUMBER, (NUMBER,), NO_CONTEXT),
  'ceiling' : (lambda s: float( s==int(s) and s or (s<=0 and int(s) or int(s)+1)),
	       NUMBER, (NUMBER,), NO_CONTEXT),
  'round' : (round_fun, NUMBER, (NUMBER,), NO_CONTEXT),
  # number operators
  'add' : (lambda s1, s2: s1 + s2, NUMBER, (NUMBER,NUMBER), NO_CONTEXT),
  'sub' : (lambda s1, s2: s1 - s2, NUMBER, (NUMBER,NUMBER), NO_CONTEXT),
  'mult' : (lambda s1, s2: s1 * s2, NUMBER, (NUMBER,NUMBER), NO_CONTEXT),
  'div' : (lambda s1, s2: s1 / s2, NUMBER, (NUMBER,NUMBER), NO_CONTEXT),
  'quo' : (lambda s1, s2: float(int(s1/s2)), NUMBER, (NUMBER,NUMBER), NO_CONTEXT),
  'mod' : (lambda s1, s2: s1 - s2*float(int(s1/s2)), NUMBER, (NUMBER,NUMBER), NO_CONTEXT),
  'neg' : (lambda s: -s, NUMBER, (NUMBER,), NO_CONTEXT),
  # comparisons
  'Lt': (lambda s: lambda o1,o2, e=s, c=lambda n1, n2: n1 < n2: relationalCompare(e,c,o1,o2)
	 , BOOLEAN, (UNKNOWN,UNKNOWN), EXPR),
  'Le': (lambda s: lambda o1,o2, e=s, c=lambda n1, n2: n1 <= n2: relationalCompare(e,c,o1,o2)
	 , BOOLEAN, (UNKNOWN,UNKNOWN), EXPR),
  'Gt': (lambda s: lambda o1,o2, e=s, c=lambda n1, n2: n1 > n2: relationalCompare(e,c,o1,o2),
	  BOOLEAN, (UNKNOWN,UNKNOWN), EXPR),
  'Ge': (lambda s: lambda o1,o2, e=s, c=lambda n1, n2: n1 >= n2: relationalCompare(e,c,o1,o2)
	 , BOOLEAN, (UNKNOWN,UNKNOWN), EXPR),
  'Eq': (lambda s: lambda o1,o2, e=s, c=lambda n1, n2: n1 == n2: equalityCompare(e,c,o1,o2)
	 , BOOLEAN, (UNKNOWN,UNKNOWN), EXPR),
  'Ne': (lambda s: lambda o1,o2, e=s, c=lambda n1, n2: n1 != n2: equalityCompare(e,c,o1,o2)
	 , BOOLEAN, (UNKNOWN,UNKNOWN), EXPR),
  # system functions
  'system_property': (lambda e, s: lambda name, e=e: system_property_fun(name,e),
		      STRING, # FIXME: may change
		      (STRING,), ENV_CONTEXT),
  'function_available': (lambda e, s: lambda name, e=e, s=s:
			                 s.locateExtension(name,e) and 1 or 0,
			 BOOLEAN, (STRING,), ENV_CONTEXT),
  'generate-id' : (lambda n,nl,e,s: OptCurrentNode(n,
						  lambda n,f: "ID%d" % id(f.normNode(n)),
						  s.factory),
		  STRING, ((NODESET,0,1),), NODE),
  }
  


def checkArgtypes(argtypes,args):
  """checks, whether *args* matches *argtype*.

  *argtypes* is a sequence of argument types.
  An argument type is either a type or a tuple
  consisting of a type, a minimal number and an optimal
  maximal number. Currently, a tuple is only allowed
  as last argument type.

  'checkArgtypes' raises a 'TypeError', if *args* does
  not match *argtypes*. Otherwise, a sequence of
  types is returned."""
  it= 0; nt= len(argtypes); na= len(args)
  if nt == 0:
    if na > 0: raise TypeError, "too many arguments"
    return ()
  while it < nt-1:
    t= argtypes[it]
    if type(t) is not IntType:
      raise ValueError, "checkArgtypes: only last type may be a tuple"
    if it >= na: raise TypeError, "too few arguments"
    # might check some type incompatibilities here.
    it= it+1
  t= argtypes[-1]; min= 1; max= 1;
  if type(t) is not IntType:
    try: (t,min,max)= t
    except ValueError:
      try: (t,min)= t; max= min
      except ValueError: raise ValueError, "checkArgtypes: argument type must be a type, a tuple or triple."
  ra= na-it
  if ra < min: raise TypeError, "too few arguments"
  if ra > max: raise TypeError, "too many arguments"
  return tuple(argtypes[:-1]) + (t,)*ra


class Constant(Expr):
  """a constant Expr."""
  def __init__(self,type,val,factory):
    self.val= val
    Expr.__init__(self,type,NO_CONTEXT,factory)
  #
  def eval(self,node,nodelist,env): return self.val


class Variable(Expr):
  """a variable."""
  def __init__(self,name,factory):
    self.name= name
    Expr.__init__(self,UNKNOWN,ENV_CONTEXT,factory)
  def eval(self,node,nodelist,env):
    v= env.getVariable(self.name)
    if v is None: raise ValueError,"undefined variable: %s" % self.name
    return v



class BoolOpExpr(Expr):
  """class for 'and' and 'or' operators."""
  def __init__(self,destval,factory):
    """*destval* is '1' for 'and' and '0' for 'or'."""
    self.destval= destval
    self.ops= []
    Expr.__init__(self,BOOLEAN,NO_CONTEXT,factory)
  #
  def add(self,expr):
    self.ops.append(expr)
    self.needed_context= max(self.needed_context,expr.needed_context)
  #
  def eval(self,node,nodelist,env):
    destval= self.destval
    for op in self.ops:
      if op.evalForType(BOOLEAN,node,nodelist,env) != destval:
	return not destval
    return destval


class EqExpr(Expr):
  """Equality Evaluation."""
  def __init__(self,op1,op2,factory):
    self.ops= [op1,op2]
    Expr.__init__(self,
		  BOOLEAN,
		  max(op1.needed_context,op2.needed_context),
		  factory)
  #
  def eval(self,node,nodelist,env):
    # check the types, collect unknowns
    ops= self.ops
    unknowns= {}; type= 1000; i= 0;
    for op in ops:
      if op.type == UNKNOWN: unknowns[i]= op
      else: type= min(type,op.type)
      i= i+1
    if type != BOOLEAN and unknowns:
      for (i,u) in unknowns.items():
	v= u.eval(node,nodelist,env)
	unknowns[i] = v
	type= min(type,classify(v))
	if type == BOOLEAN: break
      if type > STRING: type = STRING
      try: v= ops[0].convertToType(type,unknowns[0])
      except KeyError: v= ops[0].evalForType(type,node,nodelist,env)
      i= 1
      for op in ops[1:]:
	try: v1= ops[i].convertToType(type,unknowns[i])
	except: v1= op.evalForType(type,node,nodelist,env)
	if v != v1: return 0
      return 1
    if type > STRING: type = STRING
    v= ops[0].evalForType(type,node,nodelist,env)
    for op in ops[1:]:
      if op.evalForType(type,node,nodelist,env) != v: return 0
    return 1


class NodeListExpr(Expr):
  """NodeList expression."""
  def __init__(self,needed_context,factory):
    Expr.__init__(self,NODELIST,needed_context,factory)
  #
  def evalForType(self,type,node,nodelist,env):
    if type == BOOLEAN: return self.evalForNonEmpty(node,nodelist,env)
    elif type == NODELIST or type == UNKNOWN: return self.eval(node,nodelist,env)
    return self.convertToType(type,self.evalForFirst(node,nodelist,env))
  #
  def evalForNonEmpty(self,node,nodelist,env):
    return len(self.eval(node,nodelist,env)) != 0
  #
  def evalForFirst(self,node,nodelist,env):
    return self.eval(node,nodelist,env)[:1]


class FilterExpr(NodeListExpr):
  """Filter Expression."""
  # optimization chance: propagate across unions and selections
  def __init__(self,base,factory):
    self.base= base
    self.filters= []
    self.filter_context= NO_CONTEXT
    NodeListExpr.__init__(self,base.needed_context,factory)
  #
  def eval(self,node,nodelist,env):
    ns= self.base.eval(node,nodelist,env)
    return _filterAll(self.filters,ns,env)
  #
  def evalForNonEmpty(self,node,nodelist,env):
    if self.filter_context > NODEPOSITION:
      return not not self.eval(self,node,nodelist,env)
    ns= self.base.eval(node,nodelist,env)
    filters= self.filters; n= len(filters)
    fi= [0] * n; l= []
    for node in ns:
      i= 0
      for f in filters:
	if _filter(f,n,l,fi[i]+1,env):
	  fi[i]= fi[i]+1
	  if i == n-1: return 1
	else: break
        i= i+1
    return 0
  #
  def evalForFirst(self,node,nodelist,env):
    if self.filter_context > NODEPOSITION:
      return self.eval(self,node,nodelist,env)[0:1]
    ns= self.base.eval(node,nodelist,env)
    filters= self.filters; n= len(filters)
    fi= [0] * n; l= []
    for node in ns:
      i= 0
      for f in filters:
	if _filter(f,n,l,fi[i]+1,env):
	  fi[i]= fi[i]+1
	  if i == n-1: return [node]
	else: break
        i= i+1
    return []
  #
  def addFilter(self,filter):
    self.filters.append(filter)
    # ATTENTION: wrong!
    fc= filter.needed_context
    if self.needed_context < ENV_CONTEXT and fc >= ENV_CONTEXT:
      self.needed_context= ENV_CONTEXT
    if fc < NODEPOSITION and filter.type in (UNKNOWN, NUMBER):
      fc= NODEPOSITION
    self.filter_context= max(self.filter_context,fc)

def _filter(filter,node,nodelist,index,env):
  if filter.type == NUMBER or filter.type == UNKNOWN:
    v= filter.eval(node,nodelist,env)
    if type(v) == FloatType: return v == index
    return filter.convertToType(BOOLEAN,v)
  else: return filter.evalForType(BOOLEAN,node,nodelist,env)
 
def _filterAll(filters,nodelist,env):
    for f in filters:
      s= []; i= 1
      for n in nodelist:
	if _filter(f,n,nodelist,i,env): s.append(n)
	i= i+1
      nodelist= s
    return nodelist


class UnionExpr(NodeListExpr):
  """a union expression."""
  def __init__(self,factory):
    self.pathes= None; self.non_pathes= []
    NodeListExpr.__init__(self,NO_CONTEXT,factory)
  #
  def union(self,expr):
    self.needed_context= max(self.needed_context, expr.needed_context)
    if isinstance(expr,LocPathUnionExpr):
      if self.pathes is not None: self.pathes.union(expr)
      else: self.pathes= expr
    else: self.non_pathes.append(expr)
  #
  def addPath(self,locpath):
    # optimization
    if self.pathes is not None: self.pathes.addPath(locpath)
    np= self.non_pathes; i= 0; n= len(self.non_pathes)
    while i < n:
      np[i]= self.factory.addPathToExpr(np[i],locpath)
  #
  def eval(self,node,nodelist,env):
    ops= [self.pathes] + self.non_pathes
    ops= map(lambda o, n=node, ns=nodelist, e= env: o.eval(n,ns,e), ops)
    return self.factory.sortDocumentOrder(ops,env)
  #
  def evalForNonEmpty(self,node,nodelist,env):
    if self.pathes and self.pathes.evalForNonEmpty(node,nodelist,env):
      return 1
    for e in self.non_pathes:
      if e.evalForNonEmpty(node,nodelist,env): return 1
    return 0
  #
  def evalForFirst(self,node,nodelist,env):
    ops= [self.pathes] + self.non_pathes
    ops= map(lambda o, n=node, ns=nodelist, e= env: o.evalForFirst(n,ns,e), ops)
    return self.factory.minDocumentOrder(ops,env)


class TailPathExpr(NodeListExpr):
  """a path at the end of a node set expression."""
  def __init__(self,base,path,factory):
    self.base= base; self.path= path
    NodeListExpr.__init__(self,base.needed_context,factory)
  #
  def eval(self,node,nodelist,env):
    return self.path.evalSet(self.base.eval(node,nodelist,env),env)
  #
  def evalForNonEmpty(self,node,nodelist,env):
    return self.path.evalSetForNonEmpty(self.base.eval(node,nodelist,env),env)
  #
  def evalForFirst(self,node,nodelist,env):
    return self.path.evalSetForFirst(self.base.eval(node,nodelist,env),env)


class LocPathUnionExpr(NodeListExpr):
  """a union of location pathes
     optimized handling of location pathes."""
  def __init__(self,absolute,factory):
    self.abspathes= []; self.relpathes= []
    if absolute: self.abspathes.append([])
    else: self.relpathes.append([])
    NodeListExpr.__init__(self,NODE,factory)
  #
  def addStep(self,step):
    for p in self.abspathes + self.relpathes: p.append(step)
  #
  def addPath(self,locpath):
    np= locpath.getPath()
    for p in self.abspathes + self.relpathes: p[len(p):0]= np
  #
  def union(self,locpath):
    p= self.abspathes
    p[len(p):0]= locpath.abspathes
    p= self.relpathes
    p[len(p):0]= locpath.relpathes
  #
  def getPath(self):
    """must contain a single path; returns it."""
    assert not self.abspathes and len(self.relpathes) == 1
    return self.relpathes[0]
  #
  def makeAbsolute(self):
    assert not self.abspathes
    self.abspathes= self.relpathes; self.relpathes= []
  #
  # evaluations
  def eval(self,node,nodelist,env): return self.evalSet(node,env)
  def evalForNonEmpty(self,node,nodelist,env):
    return self.evalSetForNonEmpty(node,env)
  def evalForFirst(self,node,nodelist,env):
    return self.evalSetForFirst(node,env)
  def evalSet(self,node_or_list,env):
    p= EvalProcessor(self.factory)
    return p.eval(node_or_list,self.abspathes,self.relpathes,env)
  def evalSetForNonEmpty(self,node_or_list,env):
    p= EvalForNonEmptyProcessor(self.factory)
    return p.eval(node_or_list,self.abspathes,self.relpathes,env)
  def evalSetForFirst(self,node_or_list,env):
    p= EvalForFirstProcessor(self.factory)
    return p.eval(node_or_list,self.abspathes,self.relpathes,env)


class TopExpr(Expr):
  """a top level expression -- used to add parsecontext to
     evaluation context."""
  def __init__(self,expr,parsecontext):
    self._expr= expr; self._parsecontext= parsecontext
    Expr.__init__(self,expr.type,expr.needed_context,expr.factory)
  #
  def eval(self,node,nodelist,env):
    return self._expr.eval(node,nodelist,self.factory.buildEvalContext(env,self._parsecontext))
  #
  def evalForType(self,type,node,nodelist,env):
    return self._expr.evalForType(type,node,nodelist,self.factory.buildEvalContext(env,self._parsecontext))


class EvalContext:
  """evaluation context."""
  def __init__(self,env,parsecontext):
    self._env= env; self._parsecontext= parsecontext
  #
  def getNamespace(self,prefix): return self._parsecontext._resolveNamespace(prefix)
  #
  def getFunction(self,name): return self._parsecontext._resolveFunction(name)
  #
  def getVariable(self,name): return self._env.getVariable(name)


class UnOptimizable(Exception): pass

class FilterContext:
  """Nodelist context for optimized filter evaluation."""
  def __init__(self,step,env):
    self.needed_context= nc= step.filter_context
    if nc >= NODELIST or not FROM_CHILDREN<=step.axis<=FROM_SELF:
      raise UnOptimizable
    self.env= env
    self.axis= step.axis
    self.nodetest= step.nodetest
    self.filters= filters= step.filters
    if nc == NODEPOSITION: self.context= [0] * len(filters)
  #
  def eval(self,node):
    """evaluates *node* in this context.

    It returns '0', if the leading step does not match;
    '1', if it does."""
    if not self.nodetest(node,self.env): return 0
    nc= self.needed_context
    filters= self.filters; env= self.env
    if nc <= NODE:
      # we do not need nodelists for evaluation
      for f in filters:
	if not _filter(f,node,None,None,env): return 0
      return 1
    # we need node position
    i= 0; n= len(filters); context= self.context
    while i < n:
      f= filters[i]; level= context[i]= context[i] + 1
      if not _filter(f,node,[None]*(level-1) + [node], level, env): return 0
      i= i+1
    return 1


class EvalProcessor:
  """optimized evaluation of 'LocPathUnionExpr'.

  It works with a working list of nodes to visit
  and an associated dictionary keyed by nodes.
  The value of a node is the set of pathes to
  be checked for the node.

  For each node, the subtree of node is visited
  in document order. When a node is visited in the subtree,
  it is checked whether the dictionary contains
  a path set for this node. If so, the set is
  added to the pathes currently checked and
  deleted in the dictionary.
  In this way, checks for other working list nodes
  can be done as side effect of this nodes traversal,
  reducing the number of tree traversals necessary.
  Results found are added to the result set.

  If a node in the working list has no associated
  dictionary entry, then it has been processed as
  side effect of another traversal. It is simply
  discarded.

  If, during the tree traversal, a step is encountered
  that cannot be evaluated be the information available
  in the tree, the method 'scheduleUnoptimized'
  places the affected node and path on the working list.

  Tree traversal optimization is highest, if the worklist
  is kept sorted in document order.
  This, however, is not yet implemented. We try, however,
  to use not a too bad order.
  """
  def __init__(self,factory):
    self.factory= factory
  #
  class _FoundSome(Exception): pass
  class _FoundFirst(Exception): pass
  #
  def eval(self,node_or_list,abspathes,relpathes,env):
    self.env= env
    nodelist= node_or_list
    if type(nodelist) is InstanceType: nodelist= (nodelist,)
    #
    wl= self._worklist= []; self._workdict= {}
    if abspathes:
      for n in nodelist:
	doc= self.factory.getOwnerDocument(n)
	if self._knownWorkNode(doc): continue
	self._addWork(doc,abspathes)
    if relpathes:
      for n in nodelist: self._addWork(n,relpathes)
    #
    self._results= []
    #
    try: 
      while wl:
	n= wl[0]; del wl[0]
	if not self._knownWorkNode(n): continue
	r= self._result= []
	try: self._visit(n,[])
	except self._FoundFirst: pass
	if r: self._results.append(r)
    except self._FoundSome: return 1
    return self._Result()
  #
  # methods to be overwritten by descendants for
  #   nonempty and first evaluation.
  def _Result(self):
    return self.factory.sortDocumentOrder(self._results,self.env)
  #
  def _found(self,node):
    self._result.append(node)
  #
  #
  def _knownWorkNode(self,node):
    return self._workdict.has_key(self.factory.normNode(node))
  def _addWork(self,node,pathes):
    nnode= self.factory.normNode(node)
    try:
      p= self._workdict[nnode]
      p[len(p):]= pathes
    except KeyError:
      self._worklist.append(node)
      self._workdict[nnode]= pathes
  def _getWork(self,node):
    """returns work for *node* and deletes it."""
    node= self.factory.normNode(node)
    try: 
      work= self._workdict[node]; del self._workdict[node]; return work
    except KeyError: pass
  #
  # visit
  def _visit(self,node,pathes):
    newwork= self._getWork(node)
    if newwork:
      pathes= pathes[:]
      for p in newwork:
	if not p: pathes.append(p); continue
	try: pathes.append([FilterContext(p[0],self.env)] + p[1:])
	except UnOptimizable: self.scheduleUnoptimized(node,p)
    found= 0; cp= []; ap= []
    for p in pathes:
      if not p:
	if not found: found=1; self._found(node)
	continue
      while 1:
	s= p[0]; axis= s.axis
	if axis == FROM_ATTRIBUTES: s.axis= FROM_SELF; ap.append(p); break
	if axis == FROM_CHILDREN: s.axis= FROM_SELF; cp.append(p); break
	if axis == FROM_DESCENDANTS: s.axis= FROM_DESCENDANTS_OR_SELF; cp.append(p); break
	if axis in [FROM_SELF, FROM_DESCENDANTS_OR_SELF]:
	  if axis == FROM_DESCENDANTS_OR_SELF: cp.append(p)
	  if s.eval(node):
	    if len(p) == 1:
	      if not found: found=1; self._found(node)
	      break
	    try: p= [FilterContext(p[1],self.env)] + p[2:]
	    except UnOptimizable: self.scheduleUnoptimized(node,p[1:]); break
	  else: break
	else: raise SystemError, "should not arrive here"
    if ap:
      for a in self.factory.axisfun_attribute(node): self._visit(a,ap)
    if cp:
      for c in self.factory.axisfun_child(node): self._visit(c,cp)
  #
  #
  def scheduleUnoptimized(self,node,path):
    """the unoptimized evaluation of the first step of *path*."""
    s= path[0]; del path[0]
    nl= _filterAll(s.filters,s.basenodes(node,self.env),self.env)
    if not s.docOrdered(): nl.reverse()
    for n in nl: self._addWork(n,[path])

class EvalForNonEmptyProcessor(EvalProcessor):
  def _Result(self):
    return 0
  #
  def _found(self,node):
    raise self._FoundSome()

class EvalForFirstProcessor(EvalProcessor):
  def _Result(self):
    return self.factory.minDocumentOrder(self._results,self.env)
  #
  def _found(self,node):
    self._result.append(node)
    raise self._FoundFirst()



class Step:
  """a step in a location path -- factory implemented."""
  def __init__(self,axis,axisfun,nodetest,filters):
    self.axis= axis; self.axisfun= axisfun; self.nodetest= nodetest;
    self.filters= filters
    self.filter_context= NO_CONTEXT
    for f in filters:
      fc= f.needed_context
      if fc < NODEPOSITION and f.type in (UNKNOWN,NUMBER):
	fc= NODEPOSITION
      self.filter_context= max(self.filter_context,fc)
  #
  def docOrdered(self): return self.axis > 0
  #
  def basenodes(self,node,env):
    return self.axisfun(node,CheckEnv(self.nodetest,env))
  # ATTENTION: optimization potential -- adding filters


class CheckEnv:
  """check nodetest with environment."""
  def __init__(self,nodetest,env):
    self.nodetest= nodetest; self.env= env
  def __call__(self,node):
    return self.nodetest(node,self.env)


def _genAxis(axis,fun):
  return lambda self, nodetest, filters, axis=axis, fun=fun: \
				Step(axis,getattr(self,fun),nodetest,filters)

class UnimplementedExpr(Expr):
  def __init__(self,pref,name,factory):
    if prefix: name = "%s:%s" % (prefix,name)
    self.name= name
    Expr.__init__(self,UNKNOWN,NO_CONTEXT,factory)
  #
  def eval(self,node,nodelist,env):
    raise ValueError, "%s is not implemented" % self.name


class _Unimplemented:
  def __init__(self,name,factory):
    self.name= name; self.factory= factory
  def __call__(self,*args): return UnimplementedExpr(self.name,self.factory)


class BaseFactory:
  def TopExpr(self,expr,parsecontext): return TopExpr(expr,parsecontext)
  buildEvalContext= EvalContext
  def LocationPath(self,absolute): return LocPathUnionExpr(absolute,self)
  def makeAbsolute(self,expr): expr.makeAbsolute(); return expr
  def addStep(self,expr,step): expr.addStep(step); return expr
  def addDescendantsPath(self,expr,path):
    expr.addStep(self.descendants()); expr.addPath(path)
    return expr
  def addDescendantsStep(self,expr,step):
    expr.addStep(self.descendants()); expr.addStep(step)
    return expr
  def addPathToExpr(self,expr,path):
    try: addPath= expr.addPath
    except AttributeError:
      return TailPathExpr(expr,path,self)
    addPath(path)
    return expr
  def addDescendantsPathToExpr(self,expr,path):
    return self.addPathToExpr(self.addPathToExpr(expr,self.descendantsPath()),path)
  def descendants(self):
    try: return self._descendants
    except AttributeError:
      self._descendants= Step(FROM_DESCENDANTS_OR_SELF,
			      self.axisfun_descendant_or_self,
			      self.node(),
			      ())
      return self._descendants
  def descendantsPath(self):
    try: return self._descendantsPath
    except AttributeError:
      self._descendantsPath= self.LocationPath(0)
      self._descendantsPath.addStep(self.descendants())
      return self._descendantsPath
  def union(self,u1,u2):
    if hasattr(u1,'union'): u1.union(u2); return u1
    elif hasattr(u2,'union'): u2.union(u1); return u2
    u= UnionExpr(self); u.union(u1); u.union(u2); return u
  def filter(self,expr,filter):
    if not hasattr(expr,'addFilter'): expr= FilterExpr(expr,self)
    expr.addFilter(filter)
    return expr
  def Number(self,val): return Constant(NUMBER,val,self)
  def String(self,val): return Constant(STRING,val,self)
  def VariableReference(self,name): return Variable(name,self)
  def Imp_or(self,op1,op2):
    if isinstance(op1,BoolOpExpr) and op1.destval == 0: op1.add(op2); return op1
    e= BoolOpExpr(0,self); e.add(op1); e.add(op2); return e
  def Imp_and(self,op1,op2):
    if isinstance(op1,BoolOpExpr) and op1.destval == 1: op1.add(op2); return op1
    e= BoolOpExpr(1,self); e.add(op1); e.add(op2); return e
  #
  # axis functions
  ancestor= _genAxis(FROM_OTHER,'axisfun_ancestor')
  ancestor_or_self= _genAxis(FROM_OTHER,'axisfun_ancestor_or_self')
  attribute= _genAxis(FROM_ATTRIBUTES,'axisfun_attribute')
  child= _genAxis(FROM_CHILDREN,'axisfun_child')
  descendant= _genAxis(FROM_DESCENDANTS,'axisfun_descendant')
  descendant_or_self= _genAxis(FROM_DESCENDANTS_OR_SELF,'axisfun_descendant_or_self')
  following= _genAxis(FROM_FOLLOWING,'axisfun_following')
  following_sibling= _genAxis(FROM_FOLLOWING_SIBLINGS,'axisfun_following_sibling')
  parent= _genAxis(FROM_OTHER,'axisfun_parent')
  preceding= _genAxis(FROM_OTHER,'axisfun_preceding')
  preceding_sibling= _genAxis(FROM_OTHER,'axisfun_preceding_sibling')
  self= _genAxis(FROM_SELF,'axisfun_self')
  #
  # node tests
  def node(self): return lambda node, env: 1
  # other node tests are abstract
  #
  #
  def union(self,op1,op2):
    if isinstance(op1,LocPathUnionExpr) and isinstance(op2,LocPathUnionExpr):
      op1.union(op2); return op1
    if isinstance(op1, UnionExpr): op1.union(op2); return op1
    if isinstance(op2, UnionExpr): op2.union(op1); return op2
    n= UnionExpr(self); n.union(op1); n.union(op2)
    return n
  #
  #
  def Unimplemented(self,name): return _Unimplemented(name,self)
  #
  # build function
  def Function(self,desc,args): return FunCallExpr(desc,args,self)
  #
  # operators
  for __op in ('Eq', 'Ne', 'Lt', 'Gt', 'Le', 'Ge', 'add', 'sub', 'neg', 'mult',
	     'div', 'mod', 'quo'):
    exec("def %s(self,*args): return FunCallExpr(BaseFunctions['%s'],args,self)" % (__op, __op))
  del __op
  #
  # Functions
  _Functions= BaseFunctions
  _functions= None
  def _lookupFunction(self,prefix,name):
    if prefix: name= "%s:%s" % (prefix,name)
    if self._functions is None:
      f= {}
      cl= list(bases(self.__class__)); cl.reverse(); cl.append(self.__class__)
      for c in cl:
	if c.__dict__.has_key('_Functions'): f.update(c.__dict__['_Functions'])
      self._functions= f
    return self._functions.get(name)

  #
  # normalize node
  def normNode(self,node): return node
