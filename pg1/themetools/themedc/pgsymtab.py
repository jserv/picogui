#!/usr/bin/python
from fileinput import input
from string import capitalize, maketrans, translate
from re import compile
from types import IntType

pgconstant=compile('^#define\\s+((PG|GL)[_A-Z0-9]+)\\s+(\\S+)\\s*(/[*/].*)?$')
namify=maketrans('ABCDEFGHIJKLMNOPQRSTUVWXYZ_', 'abcdefghijklmnopqrstuvwxyz.')

globals={}
constants={}
properties={}
themeobjects={}
gropname={}

fullline=''
for line in input():
  if fullline:
    fullline=fullline[:-2]+line
  else:
    fullline=line
  if(len(fullline)>=2 and fullline[-2]!='\\'):
    m=pgconstant.match(line)
    if(m):
      value=eval(m.group(3), globals, constants)
      if(type(value) == IntType):
	if not constants.has_key(value):
	  constants[value]=m.group(1)	# first define regarded as authoritative
	constants[m.group(1)]=value
	constants[value]=m.group(1)
	name=translate(m.group(1), namify)
	if(name[:7]=='pgth.o.'):
	  themeobjects[value]=name[7:]
	if(name[:7]=='pgth.p.'):
	  properties[value]=name[7:]
	if(name[:8]=='pg.grop.'):
	  if(name[8:10]=='gl'):
            gropname[value]='GL'+capitalize(name[10:]);
          else:
	    if(name[8:11]=='set'):
	      gropname[value]=capitalize(name[8:11])+capitalize(name[11:])
	    else:
	      gropname[value]=capitalize(name[8:])
      else:
	print "# Couldn't eval", m.group(2), "result", repr(value), "in", fullline
    fullline=''

print "constants =", repr(constants)
print "properties =", repr(properties)
print "themeobjects =", repr(themeobjects)
print "gropname =", repr(gropname)
