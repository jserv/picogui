"""SCons.Tool.dvipdf

Tool-specific initialization for dvipdf.

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

__revision__ = "src/engine/SCons/Tool/dvipdf.py 0.D013 2003/03/31 21:46:41 software"

import SCons.Defaults

def generate(env, platform):
    """Add Builders and construction variables for dvipdf to an Environment."""
    try:
        bld = env['BUILDERS']['PDF']
    except KeyError:
        bld = SCons.Defaults.PDF()
        env['BUILDERS']['PDF'] = bld
    bld.add_action('.dvi', '$PDFCOM')

    env['DVIPDF']      = 'dvipdf'
    env['DVIPDFFLAGS'] = ''
    env['PDFCOM']      = '$DVIPDF $DVIPDFFLAGS $SOURCES $TARGET'

def exists(env):
    return env.Detect('dvipdf')
