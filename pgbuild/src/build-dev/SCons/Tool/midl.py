"""SCons.Tool.midl

Tool-specific initialization for midl (Microsoft IDL compiler).

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

__revision__ = "src/engine/SCons/Tool/midl.py 0.D013 2003/03/31 21:46:41 software"

import SCons.Defaults
import SCons.Scanner.IDL
import os.path

def midl_emitter(target, source, env):
    """Produces a list of outputs from the MIDL compiler"""
    base, ext = os.path.splitext(str(source[0]))
    tlb = base + '.tlb'
    incl = base + '.h'
    interface = base + '_i.c'
    proxy = base + '_p.c'
    dlldata = base + '_data.c'

    t = [tlb, incl, interface, proxy, dlldata]

    return (t,source)

idl_scanner = SCons.Scanner.IDL.IDLScan()

midl_builder = SCons.Builder.Builder(action='$MIDLCOM',
                                     src_suffix = '.idl',
                                     suffix='.tlb',
                                     emitter = midl_emitter,
                                     scanner = idl_scanner)

def generate(env):
    """Add Builders and construction variables for midl to an Environment."""

    env['MIDL']          = 'MIDL.EXE'
    env['MIDLFLAGS']     = '/nologo'
    env['MIDLCOM']       = "$MIDL $MIDLFLAGS /tlb ${TARGETS[0]} /h ${TARGETS[1]} /iid ${TARGETS[2]} /proxy ${TARGETS[3]} /dlldata ${TARGETS[4]} $SOURCE 2> NUL"
    env['BUILDERS']['TypeLibrary'] = midl_builder

def exists(env):
    return env.Detect('midl')
