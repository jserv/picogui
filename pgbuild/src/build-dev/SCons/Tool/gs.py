"""SCons.Tool.gs

Tool-specific initialization for Ghostscript.

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

__revision__ = "src/engine/SCons/Tool/gs.py 0.D014 2003/05/21 13:50:45 software"

import SCons.Defaults
import SCons.Platform

# Ghostscript goes by different names on different platforms...
platform = SCons.Platform.platform_default()

if platform == 'os2':
    gs = 'gsos2'
elif platform == 'win32':
    gs = 'gswin32c'
else:
    gs = 'gs'

def generate(env):
    """Add Builders and construction variables for Ghostscript to an
    Environment."""
    try:
        bld = env['BUILDERS']['PDF']
    except KeyError:
        bld = SCons.Defaults.PDF()
        env['BUILDERS']['PDF'] = bld

    bld.add_action('.ps', '$GSCOM')

    env['GS']      = gs
    env['GSFLAGS'] = '-dNOPAUSE -dBATCH -sDEVICE=pdfwrite'
    env['GSCOM']   = '$GS $GSFLAGS -sOutputFile=$TARGET $SOURCES'


def exists(env):
    if env.has_key('PS2PDF'):
        return env.Detect(env['PS2PDF'])
    else:
        return env.Detect(gs) or SCons.Util.WhereIs(gs)
