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

operatorprecedence=(('U', '!'), ('L', '*', '/'), ('L', '+', '-'),
    ('L', '<<', '>>'), ('L', '>', '<'), ('L', '=='), ('L', '&'),
    ('L', '|'), ('L', '&&'), ('L', '||'), ('R', 'questioncolon'))
def precedence(operator):
  i=0
  while i<len(operatorprecedence):
    if operator in operatorprecedence[i]:
      return i
    i=i+1
  raise 'unknown operator %s'%operator
fsoperators={constants['PGTH_OPCMD_PLUS']: '+',
    constants['PGTH_OPCMD_MINUS']: '-', constants['PGTH_OPCMD_MULTIPLY']: '*',
    constants['PGTH_OPCMD_SHIFTL']: '<<', constants['PGTH_OPCMD_SHIFTR']: '>>',
    constants['PGTH_OPCMD_OR']: '|', constants['PGTH_OPCMD_AND']: '&',
    constants['PGTH_OPCMD_EQ']: '==', constants['PGTH_OPCMD_DIVIDE']: '/',
    constants['PGTH_OPCMD_GT']: '>', constants['PGTH_OPCMD_LOGICAL_OR']: '||',
    constants['PGTH_OPCMD_LOGICAL_AND']: '&&', constants['PGTH_OPCMD_LT']: '<'}
# Note: argument type list is in reverse.
fsfunctions={constants['PGTH_OPCMD_COLORADD']: ('ColorAdd', 'color', 'color'),
    constants['PGTH_OPCMD_COLORSUB']: ('ColorSub', 'color', 'color'),
    constants['PGTH_OPCMD_COLORDIV']: ('ColorDiv', 'color', 'color'),
    constants['PGTH_OPCMD_COLORMULT']: ('ColorMult', 'color', 'color')}

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

filenumber=0
userprops=[]

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
    return 'themeauto%d'%(value-constants['PGTH_P_THEMEAUTO'])
  elif value > constants['PGTH_P_USER']:
    global userprops
    for p in userprops:
      if p[1]==value:
        return p[0]
    name='userprop%d'%(value-constants['PGTH_P_USER'])
    userprops.append((name, value))
    return name
  else:
    return str(value)

def lookup_objname(value):
  if themeobjects.has_key(value):
    return themeobjects[value]
  else:
    return str(value)

def quote_string(string):
  string=replace(string, '\\', '\\\\')
  string=replace(string, '\n', '\\n')
  string=replace(string, '"', '\\"')
  return string

PGTH_FORMATVERSION=2
ThemeHdrFmt="!4x2L4H"
ThemeObjFmt="!2HL"
ThemePropFmt="!2HL"
PgReqFmt="!2LH2x"       # current format
#PgReqFmt='!2HL'        # old format
PgReqFontFmt="!40sLHxx" # MKFONT request structure
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
  def formula(self, value, prec=len(operatorprecedence), side='N'):
    if value[1] in ('int', 'literal', 'xsize', 'ysize', 'var',
        'bitmap', 'string', 'font'):
      return str(value[0])
    elif value[1]=='direction':
      return lookup_constname('PG_DIR_', value[0])
    elif value[1]=='lgop':
      return lookup_constname('PG_LGOP_', value[0])
    elif value[1]=='glconst':
      return lookup_constname('GL_', value[0])
    elif value[1]=='color':
      if type(value[0]) == StringType:
        return value[0]
      else:
	return '0x%06x'%value[0]
    elif value[1]=='fixedpoint16':
	return '0x%x'%value[0]
    elif value[1]=='function':
      res=value[0]+'('
      for arg in value[2:]:
        res=res+self.formula(arg)+', '
      if len(value)==2:
        return res+')'
      else:
        return res[:-2]+')'
    elif value[1]=='unary':
      prec=precedence(value[0])	# unary operator never needs parenthesis
      return value[0]+self.formula(value[2], prec,
          operatorprecedence[prec][0])
    elif value[1]=='operator':
      pr=precedence(value[0])
      ret=self.formula(value[2], pr, 'L')+value[0]+self.formula(value[3],
          pr, 'R')
      if (pr>prec) or (pr==prec and side!=operatorprecedence[prec][0]):
        ret='('+ret+')'
      return ret
    elif value[1]=='questioncolon':
      pr=precedence(value[1])	# not value[0] for ?:
      # ? forces parentheses on nested ?:s for clarity
      ret="%s?%s:%s"%(self.formula(value[0], pr, 'L'),
	  self.formula(value[2], pr, '?'),
	  self.formula(value[3], pr, '?'))
      if (pr>prec) or (pr==prec and side!=operatorprecedence[prec][0]):
        ret='('+ret+')'
      return ret
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
    self.source=self.source+"\t"+gropname[gropcode]+"("
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
      arg=self.stack[-params:]
      del self.stack[-params:]
      if gropcode == constants['PG_GROP_SETCOLOR']:
        self.assigntype(arg[0],'color')
      elif gropcode == constants['PG_GROP_SETANGLE']:
        self.assigntype(arg[0],'direction')
      elif gropcode == constants['PG_GROP_SETFONT']:
        self.assigntype(arg[0],'font')
      elif gropcode == constants['PG_GROP_TEXT']:
        self.assigntype(arg[0],'string')
      elif gropcode == constants['PG_GROP_SETLGOP']:
        self.assigntype(arg[0],'lgop')
      elif gropcode in (constants['PG_GROP_BITMAP'],
                constants['PG_GROP_TILEBITMAP']):
        self.assigntype(arg[0],'bitmap')
      elif gropcode == constants['PG_GROP_GRADIENT']:
        self.assigntype(arg[0],'direction')
        self.assigntype(arg[1],'color')
        self.assigntype(arg[2],'color')
      elif gropcode == constants['PG_GROP_GL_BINDTEXTURE']:
        self.assigntype(arg[0],'bitmap')
      elif gropcode == constants['PG_GROP_GL_ENABLE']:
        self.assigntype(arg[0],'glconst')
      elif gropcode == constants['PG_GROP_GL_DISABLE']:
        self.assigntype(arg[0],'glconst')
      elif gropcode == constants['PG_GROP_GL_DEPTHFUNC']:
        self.assigntype(arg[0],'glconst')
      elif gropcode == constants['PG_GROP_GL_SHADEMODEL']:
        self.assigntype(arg[0],'glconst')
      elif gropcode == constants['PG_GROP_GL_MATRIXMODE']:
        self.assigntype(arg[0],'glconst')
      elif gropcode == constants['PG_GROP_GL_TRANSLATEF']:
        self.assigntype(arg[0],'fixedpoint16')
        self.assigntype(arg[1],'fixedpoint16')
        self.assigntype(arg[2],'fixedpoint16')
      elif gropcode == constants['PG_GROP_GL_ROTATEF']:
        self.assigntype(arg[0],'fixedpoint16')
        self.assigntype(arg[1],'fixedpoint16')
        self.assigntype(arg[2],'fixedpoint16')
      elif gropcode == constants['PG_GROP_GL_SCALEF']:
        self.assigntype(arg[0],'fixedpoint16')
        self.assigntype(arg[1],'fixedpoint16')
        self.assigntype(arg[2],'fixedpoint16')
      elif gropcode == constants['PG_GROP_GL_BEGIN']:
        self.assigntype(arg[0],'glconst')
      elif gropcode == constants['PG_GROP_GL_TEXCOORD2F']:
        self.assigntype(arg[0],'fixedpoint16')
        self.assigntype(arg[1],'fixedpoint16')
      elif gropcode == constants['PG_GROP_GL_VERTEX3F']:
        self.assigntype(arg[0],'fixedpoint16')
        self.assigntype(arg[1],'fixedpoint16')
        self.assigntype(arg[2],'fixedpoint16')
      elif gropcode == constants['PG_GROP_GL_COLOR']:
        self.assigntype(arg[0],'color')
      elif gropcode == constants['PG_GROP_GL_HINT']:
        self.assigntype(arg[0],'glconst')
        self.assigntype(arg[1],'glconst')
      elif gropcode == constants['PG_GROP_GL_NORMAL3F']:
        self.assigntype(arg[0],'fixedpoint16')
        self.assigntype(arg[1],'fixedpoint16')
        self.assigntype(arg[2],'fixedpoint16')
      elif gropcode == constants['PG_GROP_GL_LIGHTFV']:
        self.assigntype(arg[0],'fixedpoint16')
        self.assigntype(arg[1],'fixedpoint16')
        self.assigntype(arg[2],'fixedpoint16')
      elif gropcode == constants['PG_GROP_GL_BLENDFUNC']:
        self.assigntype(arg[0],'glconst')
        self.assigntype(arg[1],'glconst')
      else:
        raise "Unimplemented gropnode %s"%lookup_constname('PG_GROP_', gropcode)
      while len(arg):
        self.source=self.source+self.formula(arg.pop(0))+', '
      self.source=self.source[:-2]
    self.source=self.source+");\n"
    if not PG_GROP_IS_UNPOSITIONED(gropcode):
      self.stack.pop()          # H
      self.stack.pop()          # W
      self.stack.pop()          # Y
      self.stack.pop()          # X
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
      # For debugging.
      #print self.source,'stack:',self.stack,'\nvars:',self.localvars, \
      # 'op:',opcode
      # This should handle variables reasonably well
      while len(self.stack)<len(self.localvars):
        pop=self.localvars.pop()
        if pop[1] != 'unref':
          raise "Referenced variable %s(%s) popped!"%(pop[0],pop[1])
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
          self.stack.append([unpack('!l', bytecode[p:p+4])[0], 'literal'])
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
	elif opcode == constants['PGTH_OPCMD_LOGICAL_NOT']:
	  self.stack.append(['!', 'unary', self.stack.pop()])
        elif opcode in fsoperators.keys():
          fsb=self.stack.pop()
          fsa=self.stack.pop()
          self.stack.append([fsoperators[opcode], 'operator', fsa, fsb])
        elif opcode == constants['PGTH_OPCMD_QUESTIONCOLON']:
          fsa=self.stack.pop()
          fsb=self.stack.pop()
          fsc=self.stack.pop()
          self.stack.append([fsa, 'questioncolon', fsb, fsc])
        elif opcode in fsfunctions.keys():
          arg=[]
          for type in fsfunctions[opcode][1:]:
            arg.insert(0, self.stack.pop())
            self.assigntype(arg[0], type)
          self.stack.append([fsfunctions[opcode][0], 'function']+arg)
        else:
          raise "Unimplemented fillstyle opcmd %s"% \
                lookup_constname("PGTH_OPCMD_", opcode)
      elif opcode & constants['PGTH_OPSIMPLE_GET']:
        var=opcode&(constants['PGTH_OPSIMPLE_GET']-1)
        self.getvar(var)
      else:     # singlebyte set command
        var=opcode&(constants['PGTH_OPSIMPLE_GET']-1)
        self.setvar(var)

class PgRequest:
  def __str__(self):
    if self.type == constants['PGREQ_MKBITMAP']:
      global filenumber
      filename='themedc-bitmap%02d'%filenumber
      filenumber=filenumber+1
      open(filename,'wb').write(self.data)
      return 'LoadBitmap("%s")'%filename
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
      self.name=self.name.split('\0',1)[0]
      return self
    elif self.type == constants['PGREQ_MKSTRING']:
      return themestr[p+PgReqLen:p+PgReqLen+size]
    elif self.type == constants['PGREQ_MKARRAY']:
      return unpack("!%dL"%(size/4), themestr[p+PgReqLen:p+PgReqLen+size])
    elif self.type == constants['PGREQ_MKFILLSTYLE']:
      return PgFillstyle(themestr[p+PgReqLen:p+PgReqLen+size])
    elif self.type == constants['PGREQ_MKBITMAP']:
      self.data = themestr[p+PgReqLen:p+PgReqLen+size]
    else:
      raise "Unimplemented request %s"%lookup_constname("PGREQ_", self.type)
    return self

class PgThProp:
  def name(self):
    return lookup_propname(self.id)
  def type(self):
    return lookup_proptype(self.name())
  def __str__(self):
    me = self.name()
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
      return self       # nothing needs to be done
    elif self.loader == constants['PGTH_LOAD_REQUEST']:
      self.data=PgRequest().fromTh(themestr, self.data)
    elif self.loader == constants['PGTH_LOAD_FINDTHOBJ']:
      self.data=themestr[self.data:self.data+index(themestr[self.data:], '\0')]
    else:
      raise "Unimplemented loader %s"%lookup_constname('PGTH_LOAD_', self.loader)
    return self

class PgThObj:
  def name(self):
    return lookup_objname(self.id)
  def __str__(self):
    me='object '+self.name()
    if len(self.prop)>1:
      me=me+' {\n'
      for property in self.prop:
        me=me+str(property)
      return me+"}\n\n"
    else:
      return me+str(self.prop[0])+'\n'
  def __init__(self):
    self.id = constants['PGTH_O_CUSTOM']
    self.prop=[]
  def fromTh(self, themestr, p):
    (self.id, num_prop, proplist) = unpack(ThemeObjFmt,
        themestr[p:p+ThemeObjLen])
    p=proplist
    while num_prop:
      prop=PgThProp().fromTh(themestr, p)
      self.prop.append(prop)
      num_prop=num_prop-1
      p=p+ThemePropLen
    return self

class PgTheme:
  def __str__(self):
    me=''
    for obj in self.obj:
      me=me+str(obj)
    global userprops
    if len(userprops):
      me=';\n\n'+me
      for p in userprops:
        me=', %s=PGTH_P_USER+%d'%(p[0], p[1]-constants['PGTH_P_USER'])+me
      me='prop'+me[1:]
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

