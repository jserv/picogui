"""engine.SCons.Tool.ifl

Tool-specific initialization for the Intel Fortran compiler.

There normally shouldn't be any need to import this module directly.
It will usually be imported through the generic SCons.Tool.Tool()
selection method.

"""

#
# Copyright (c) 2001, 2002, 2003 Steven Knight
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
# KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

__revision__ = "src/engine/SCons/Tool/ifl.py 0.D014 2003/05/21 13:50:45 software"

import f77

def generate(env):
    """Add Builders and construction variables for ifl to an Environment."""
    f77.generate(env)

    env['F77']        = 'ifl'
    env['F77FLAGS']   = ''
    env['F77COM']     = '$F77 $F77FLAGS $_F77INCFLAGS /c $SOURCES /Fo$TARGET'
    env['F77PPCOM']   = '$F77 $F77FLAGS $CPPFLAGS $_F77INCFLAGS /c $SOURCES /Fo$TARGET'
    env['SHF77']      = '$F77'
    env['SHF77FLAGS'] = '$F77FLAGS'
    env['SHF77COM']   = '$SHF77 $SHF77FLAGS $_F77INCFLAGS /c $SOURCES /Fo$TARGET'
    env['SHF77PPCOM'] = '$SHF77 $SHF77FLAGS $CPPFLAGS $_F77INCFLAGS /c $SOURCES /Fo$TARGET'

def exists(env):
    return env.Detect('ifl')
