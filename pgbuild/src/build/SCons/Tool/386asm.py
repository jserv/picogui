"""SCons.Tool.386asm

Tool specification for the 386ASM assembler for the Phar Lap ETS embedded
operating system.

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

__revision__ = "src/engine/SCons/Tool/386asm.py 0.D013 2003/03/31 21:46:41 software"

import os.path
import string
import re

import SCons.Action
import SCons.Defaults
import SCons.Tool

from SCons.Tool.PharLapCommon import addPharLapPaths

ASSuffixes = ['.s', '.asm', '.ASM']
ASPPSuffixes = ['.spp', '.SPP']
if os.path.normcase('.s') == os.path.normcase('.S'):
    ASSuffixes.extend(['.S'])
else:
    ASPPSuffixes.extend(['.S'])

def generate(env, platform):
    """Add Builders and construction variables for ar to an Environment."""
    static_obj, shared_obj = SCons.Tool.createObjBuilders(env)

    for suffix in ASSuffixes:
        static_obj.add_action(suffix, SCons.Defaults.ASAction)

    for suffix in ASPPSuffixes:
        static_obj.add_action(suffix, SCons.Defaults.ASPPAction)

    env['AS']        = '386asm'
    env['ASFLAGS']   = ''
    env['ASCOM']     = '$AS $ASFLAGS $SOURCES -o $TARGET'
    env['ASPPCOM']   = '$CC $ASFLAGS $CPPFLAGS $SOURCES -o $TARGET'

    addPharLapPaths(env)

def exists(env):
    return env.Detect('386asm')
