from struct import unpack, calcsize
from types import StringType
from string import replace, index
from pgsymbols import *
from re import compile

class PM:
  def __init__(self, regexp, datatype, prefix, printfformat):
    self.re=compile(regexp)
    self.type=datatype
    self.prefix=prefix
    self.format=printfformat

propertymatch=[
	PM('^textcolors$', 'terminalpalette', None, 'Array(0x%06x, 0x%06x, 0x%06x, 0x%06x, 0x%06x,\n\t0x%06x, 0x%06x, 0x%06x, 0x%06x, 0x%06x, 0x%06x,\n\t0x%06x, 0x%06x, 0x%06x, 0x%06x, 0x%06x)'),
	PM('color$', 'color', None, '0x%06x'),
	PM('^width$', 'xsize', None, '%d'),
	PM('^height$', 'ysize', None, '%d'),
	PM('^align$', 'align', 'PG_A_', None),
	PM('(margin|offset|spacing)$', 'int', None, '%d'),
	PM('side$', 'side', 'PG_S_', None),
	PM('^time', 'millisec', None, '%d /*ms*/'),
	PM('^parent$', 'themeobj', 'PGTH_O_', None),
	PM('^hidehotkeys$', 'hidehotkeys', 'PG_HHK_', None),
	PM('^attr\.(default|cursor)$', 'terminalattr', None, '0x%02x'),
	PM('^hotkey\.', 'key', 'PGKEY_', None),
	PM('^(text|string\.|name$)', 'string', None, None),
	PM('^font$', 'font', None, None),
	PM('^icon\.', 'bitmap', None, None),
	PM('bitma(p|sk)[0-9]?$', 'bitmap', None, None),
	PM('^(bgfill|overlay|backdrop)$', 'fillstyle', None, None)]

def lookup_proptype(name):
  for pm in propertymatch:
    if pm.re.search(name):
      return pm.type
  return 'int'

def lookup_constname(prefix, value):
  for key in constants.keys():
    if type(key) == type(prefix) and key[:len(prefix)] == prefix and \
    	constants[key] == value:
      return key
  return "/* "+prefix+" */ "+str(value)

def lookup_bitmask(prefix, value):
  string=None
  for key in constants.keys():
    if type(key) == type(prefix) and key[:len(prefix)] == prefix:
      if value&constants[key] == constants[key]:
	if string:
	  string=string+'|'+key
	else:
	  string=key
	value=value&(~constants[key])
  if not string:
    string='0'
  return string

def lookup_propname(value):
  if properties.has_key(value):
    return properties[value]
  elif value > constants['PGTH_P_THEMEAUTO']:
    return 'themeauto+%d'%(value-constants['PGTH_P_THEMEAUTO'])
  elif value > constants['PGTH_P_USER']:
    return 'user+%d'%(value-constants['PGTH_P_USER'])
  else:
    return str(value)

def lookup_objname(value):
  if themeobjects.has_key(value):
    return themeobjects[value]
  else:
    return str(value)

def quote_string(string):
  replace(string, '\\', '\\\\')
  replace(string, '\n', '\\n')
  replace(string, '"', '\\"')
  return string

PGTH_FORMATVERSION=2
ThemeHdrFmt="!4x2L4H"
ThemeObjFmt="!2HL"
ThemePropFmt="!2HL"
PgReqFmt="!2LH2x"	# current format
#PgReqFmt='!2HL' 	# old format
PgReqFontFmt="!40sLHxx"	# MKFONT request structure
ThemeHdrLen=calcsize(ThemeHdrFmt)
ThemeObjLen=calcsize(ThemeObjFmt)
ThemePropLen=calcsize(ThemePropFmt)
PgReqLen=calcsize(PgReqFmt)
PgReqFontLen=calcsize(PgReqFontFmt)

def PG_GROPPARAMS(grop):
  return (grop>>2)&0x03

def PG_GROP_IS_UNPOSITIONED(grop):
  return grop&2

class PgFillstyle:
  def __str__(self):
    res='fillstyle {\n'
    for var in self.localvars[4:]:
      res=res+'\tvar '+var[0]+';\n'
    return res+self.source+'    }'
  def formula(self, value):
    if value[1] in ('int', 'literal', 'xsize', 'ysize', 'var',
    	'bitmap'):
      return str(value[0])
    elif value[1]=='direction':
      return lookup_constname('PG_DIR_', value[0])
    elif value[1]=='lgop':
      return lookup_constname('PG_LGOP_', value[0])
    elif value[1]=='color':
      if type(value[0]) == StringType:
	return value[0]
      else:
	return '0x%06x'%value[0]
    elif value[1]=='function':
      res=value[0]+'('
      for arg in value[2:]:
	res=res+self.formula(arg)+', '
      return res[:-2]+')'
    elif value[1]=='operator':
      return self.formula(value[2])+value[0]+self.formula(value[3])
    elif value[1]=='questioncolon':
      return "%s?%s:%s"%(self.formula(value[0]), self.formula(value[2]),
    		self.formula(value[3]))
    else:
      raise "don't know type %s"%value[1]
  def assigntype(self,where,type):
    if where[1] in ('operator', 'questioncolon', 'function'):
      for arg in where[2:]:
	self.assigntype(arg, type)
    elif where[1] == 'var':
      for var in self.localvars:
	if var[0]==where[0]:
	  var[1]=type
	  break
    else:
      where[1]=type
  def typeof(self, where):
    if where[1] in ('operator', 'questioncolon', 'function'):
      return 'int'
    else:
      return where[1]
  def grop(self,gropcode):
    params=PG_GROPPARAMS(gropcode)
    self.source=self.source+"\t"+fillstylefuncs[gropcode]+"("
    if not PG_GROP_IS_UNPOSITIONED(gropcode):
      self.assigntype(self.stack[-4-params], 'int')
      self.assigntype(self.stack[-3-params], 'int')
      self.assigntype(self.stack[-2-params], 'xsize')
      self.assigntype(self.stack[-1-params], 'ysize')
      self.source=self.source+self.formula(self.stack[-4-params])+", "+ \
      		self.formula(self.stack[-3-params])+", "+ \
      		self.formula(self.stack[-2-params])+", "+ \
      		self.formula(self.stack[-1-params])+", "
      if not params:
	self.source=self.source[:-2]
    if params:
      if gropcode == constants['PG_GROP_SETCOLOR']:
	value=self.stack.pop()
	self.assigntype(value,'color')
	self.source=self.source+self.formula(value)
      elif gropcode == constants['PG_GROP_SETLGOP']:
	value=self.stack.pop()
	self.assigntype(value,'lgop')
	self.source=self.source+self.formula(value)
      elif gropcode in (constants['PG_GROP_BITMAP'],
      		constants['PG_GROP_TILEBITMAP']):
	value=self.stack.pop()
	self.assigntype(value,'bitmap')
	self.source=self.source+self.formula(value)
      elif gropcode == constants['PG_GROP_GRADIENT']:
	col2=self.stack.pop()
	self.assigntype(col2,'color')
	col1=self.stack.pop()
	self.assigntype(col1,'color')
	dir=self.stack.pop()
	self.assigntype(dir,'direction')
	self.source=self.source+self.formula(dir)+", "+self.formula(col1)+ \
		", "+self.formula(col2)
      else:
	raise "Unimplemented gropnode %s"%lookup_constname('PG_GROP_', gropcode)
    self.source=self.source+");\n"
    if not PG_GROP_IS_UNPOSITIONED(gropcode):
      self.stack.pop()		# H
      self.stack.pop()		# W
      self.stack.pop()		# Y
      self.stack.pop()		# X
  def getvar(self,var):
    self.stack.append([self.localvars[var][0], 'var'])
  def setvar(self,var):
    value=self.stack.pop()
    self.assigntype(self.localvars[var], self.typeof(value))
    self.source=self.source+"\t"+self.localvars[var][0]+" = "+ \
    	self.formula(value)+";\n"
  def __init__(self, bytecode):
    self.source=''
    self.stack=[['x','var'], ['y','var'], ['w','var'], ['h','var']]
    self.localvars=[['x','int'], ['y','int'], ['w','xsize'], ['h','ysize']]
    p=0
    self.currentformula=[]
    while p<len(bytecode):
      # Hack to register local variables used by themec
      # TODO: proper naming of the vars
      opcode=ord(bytecode[p])
      p=p+1
      # This should handle variables reasonably well
      while len(self.stack)<len(self.localvars):
	self.localvars.pop()
      if len(self.stack)-3 == p and opcode==constants['PGTH_OPSIMPLE_LITERAL']:
	self.localvars.append(["l%02d"%(len(self.localvars)-3), 'unref'])
        self.stack.append([0, 'literal'])
      # Normal bytecode interpreting
      elif opcode & constants['PGTH_OPSIMPLE_GROP']:
	self.grop(opcode&(constants['PGTH_OPSIMPLE_GROP']-1))
      elif opcode & constants['PGTH_OPSIMPLE_LITERAL']:
	self.stack.append([opcode&(constants['PGTH_OPSIMPLE_LITERAL']-1), \
		'literal'])
      elif opcode & constants['PGTH_OPSIMPLE_CMDCODE']:
	if opcode == constants['PGTH_OPCMD_LONGLITERAL']:
	  self.stack.append([unpack('!L', bytecode[p:p+4])[0], 'literal'])
	  p=p+4
	elif opcode == constants['PGTH_OPCMD_LONGGROP']:
	  self.grop(unpack('!H', bytecode[p:p+2])[0])
	  p=p+2
	elif opcode == constants['PGTH_OPCMD_LONGGET']:
	  var=ord(bytecode[p])
	  p=p+1
	  self.getvar(var)
	elif opcode == constants['PGTH_OPCMD_LONGSET']:
	  var=ord(bytecode[p])
	  p=p+1
	  self.setvar(var)
	elif opcode == constants['PGTH_OPCMD_PROPERTY']:
	  # essentially same as copy loader
	  (thobj,prop)=unpack('!2H', bytecode[p:p+4])
	  p=p+4
	  thobj=lookup_objname(thobj)
	  prop=lookup_propname(prop)
	  type=lookup_proptype(prop)
	  self.stack.append([thobj+'::'+prop, type])
	elif opcode == constants['PGTH_OPCMD_LOCALPROP']:
	  prop=unpack('!H', bytecode[p:p+2])[0]
	  prop=lookup_propname(prop)
	  p=p+2
	  self.stack.append([prop,lookup_proptype(prop)])
	elif opcode == constants['PGTH_OPCMD_SHIFTR']:
	  fsb=self.stack.pop()
	  fsa=self.stack.pop()
	  self.stack.append(['>>', 'operator', fsa, fsb])
	elif opcode == constants['PGTH_OPCMD_PLUS']:
	  fsb=self.stack.pop()
	  fsa=self.stack.pop()
	  self.stack.append(['+', 'operator', fsa, fsb])
	elif opcode == constants['PGTH_OPCMD_MINUS']:
	  fsb=self.stack.pop()
	  fsa=self.stack.pop()
	  self.stack.append(['-', 'operator', fsa, fsb])
	elif opcode == constants['PGTH_OPCMD_GT']:
	  fsb=self.stack.pop()
	  fsa=self.stack.pop()
	  self.stack.append(['>', 'operator', fsa, fsb])
	elif opcode == constants['PGTH_OPCMD_DIVIDE']:
	  fsb=self.stack.pop()
	  fsa=self.stack.pop()
	  self.stack.append(['/', 'operator', fsa, fsb])
	elif opcode == constants['PGTH_OPCMD_QUESTIONCOLON']:
	  fsa=self.stack.pop()
	  fsb=self.stack.pop()
	  fsc=self.stack.pop()
	  self.stack.append([fsa, 'questioncolon', fsb, fsc])
	elif opcode == constants['PGTH_OPCMD_COLORDIV']:
	  fsb=self.stack.pop()
	  fsa=self.stack.pop()
	  self.stack.append(['ColorDiv', 'function', fsa, fsb])
	elif opcode == constants['PGTH_OPCMD_COLORSUB']:
	  fsb=self.stack.pop()
	  fsa=self.stack.pop()
	  self.stack.append(['ColorSub', 'function', fsa, fsb])
	else:
	  raise "Unimplemented fillstyle opcmd %s"% \
	  	lookup_constname("PGTH_OPCMD_", opcode)
      elif opcode & constants['PGTH_OPSIMPLE_GET']:
	var=opcode&(constants['PGTH_OPSIMPLE_GET']-1)
	self.getvar(var)
      else:	# singlebyte set command
	var=opcode&(constants['PGTH_OPSIMPLE_GET']-1)
	self.setvar(var)

class PgRequest:
  def __str__(self):
    if self.type == constants['PGREQ_MKBITMAP']:
      return 'LoadBitmap("FIXME-UNSAVED")'
    elif self.type == constants['PGREQ_MKFONT']:
      return 'Font("%s",%d,%s)'%(self.name, self.size,
	  lookup_bitmask("PG_FSTYLE_", self.style))
    else:
      raise 'PgRequest does not know how to output type %s'% \
	  lookup_constname("PGREQ_", self.type)
  def fromTh(self, themestr, p):
    # new format
    (id, size, self.type) = unpack(PgReqFmt, themestr[p:p+PgReqLen])
    # old format
    #(self.type, id, size) = unpack(PgReqFmt, themestr[p:p+PgReqLen])
    if self.type == constants['PGREQ_MKFONT']:
      (self.name, self.style, self.size) = unpack(PgReqFontFmt, \
      		themestr[p+PgReqLen:p+PgReqLen+PgReqFontLen])
      return self
    elif self.type == constants['PGREQ_MKSTRING']:
      return themestr[p+PgReqLen:p+PgReqLen+size]
    elif self.type == constants['PGREQ_MKARRAY']:
      return unpack("!%dL"%(size/4), themestr[p+PgReqLen:p+PgReqLen+size])
    elif self.type == constants['PGREQ_MKFILLSTYLE']:
      return PgFillstyle(themestr[p+PgReqLen:p+PgReqLen+size])
    elif self.type == constants['PGREQ_MKBITMAP']:
      # FIXME this is not a string... needs a file name 
      self.data = themestr[p+PgReqLen:p+PgReqLen+size]
    else:
      raise "Unimplemented request %s"%lookup_constname("PGREQ_", self.type)
    return self

class PgThProp:
  def __str__(self):
    me = lookup_propname(self.id)
    if self.loader == constants['PGTH_LOAD_FINDTHOBJ']:
      val='FindThemeObject("'+quote_string(self.data)+'")'
    elif type(self.data) == StringType:
      val='"'+quote_string(self.data)+'"'
    elif self.loader == constants['PGTH_LOAD_COPY']:
      val=lookup_objname((self.data&0xffff0000)>>16)+"::"+ \
      		lookup_propname(self.data&0xffff)
    else:
      val=str(self.data)
      for pm in propertymatch:
	if pm.re.search(me):
	  if pm.format:
	    val=pm.format%self.data
	  elif pm.prefix:
	    val=lookup_constname(pm.prefix, self.data)
	  break
    return "  "+me+" = "+val+";\n"

  def __init__(self):
    self.loader=constants['PGTH_LOAD_NONE']
  def fromTh(self, themestr, p):
    (self.id, self.loader, self.data) = unpack(ThemePropFmt, \
    	themestr[p:p+ThemePropLen])
    if self.loader == constants['PGTH_LOAD_NONE'] or \
    	self.loader == constants['PGTH_LOAD_COPY']:
      return self	# nothing needs to be done
    elif self.loader == constants['PGTH_LOAD_REQUEST']:
      self.data=PgRequest().fromTh(themestr, self.data)
    elif self.loader == constants['PGTH_LOAD_FINDTHOBJ']:
      self.data=themestr[self.data:self.data+index(themestr[self.data:], '\0')]
    else:
      raise "Unimplemented loader %s"%lookup_constname('PGTH_LOAD_', self.loader)
    return self

class PgThObj:
  def __str__(self):
    me='object '+lookup_objname(self.id)+' {\n'
    for property in self.prop:
      me=me+str(property)
    return me+"}\n\n"
  def __init__(self):
    self.id = constants['PGTH_O_CUSTOM']
    self.prop=[]
  def fromTh(self, themestr, p):
    (self.id, num_prop, proplist) = unpack(ThemeObjFmt, \
    	themestr[p:p+ThemeObjLen])
    p=proplist
    while num_prop:
      self.prop.append(PgThProp().fromTh(themestr, p))
      num_prop=num_prop-1
      p=p+ThemePropLen
    return self

class PgTheme:
  def __str__(self):
    me=''
    for obj in self.obj:
      me=me+str(obj)
    return me
  def __init__(self):
    self.obj=[]
    self.tag=[]
  def fromTh(self, themestr):
    if(themestr[:4] != 'PGth'):
      raise "Incorrect theme magic"
    (length, checksum, version, num_tags, num_thobj, num_totprop) = \
	unpack(ThemeHdrFmt, themestr[:ThemeHdrLen])
    if version != PGTH_FORMATVERSION:
      raise "PgTheme format version mismatch"
    if length != len(themestr):
      raise "Incorrect theme length"
    # verify checksum
    ourchecksum=0L
    p=0
    while p<len(themestr):
      ourchecksum=ourchecksum+ord(themestr[p])
      p=p+1
    # subtract the checksum bytes
    ourchecksum=ourchecksum-ord(themestr[8])
    ourchecksum=ourchecksum-ord(themestr[9])
    ourchecksum=ourchecksum-ord(themestr[10])
    ourchecksum=ourchecksum-ord(themestr[11])
    ourchecksum=ourchecksum&0xffffffffL
    if(checksum != ourchecksum):
      raise "Incorrect checksum"
    p=ThemeHdrLen
    while num_thobj:
      self.obj.append(PgThObj().fromTh(themestr, p))
      num_totprop=num_totprop-len(self.obj[-1].prop)
      num_thobj=num_thobj-1
      p=p+ThemeObjLen
    if num_totprop>0:
      raise "More properties than theme objects asked for"
    elif num_totprop<0:
      raise "Theme objects loaded more than num_totprop properties"
    return self

