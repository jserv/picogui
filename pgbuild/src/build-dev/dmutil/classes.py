# Copyright (C) 1997 by Dr. Dieter Maurer <dieter@hit.handshake.de>
# D-66386 St. Ingbert, Eichendorffstr. 23, Germany
#
#			All Rights Reserved
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted,
# provided that the above copyright notice appear in all copies and
# modified copies and that
# both that copyright notice and this permission notice appear in
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


# class.py -- auxiliary function for class handling
"""auxiliary functions for class handling"""

__version__="$Id: classes.py,v 1.1 1997/11/16 22:41:30 dieter Exp dieter $"

import types

def bases(cl):
  """returns the base classes of *cl*"""
  # the algorithm has quadratic time complexity
  # however, classes normally should not be too complex
  return reduce(lambda s,e: s + (e,) + bases(e),cl.__bases__,())

def is_method(o):
  """returns *true* if *o* may be a method"""
  t= type(o)
  return t is types.MethodType or t is types.FunctionType or t is types.ClassType

def isa(o,cl):
  """*o* is an instance of *cl*."""
  c= o.__class__
  return c is cl or c in bases(cl)
