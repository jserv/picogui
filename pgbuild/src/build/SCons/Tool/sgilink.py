"""SCons.Tool.sgilink

Tool-specific initialization for the SGI MIPSPro linker.

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

__revision__ = "src/engine/SCons/Tool/sgilink.py 0.D013 2003/03/31 21:46:41 software"

import SCons.Defaults
import SCons.Util

linkers = ['CC', 'cc']

def generate(env, platform):
    """Add Builders and construction variables for MIPSPro to an Environment."""
    env['BUILDERS']['SharedLibrary'] = SCons.Defaults.SharedLibrary
    env['BUILDERS']['Program'] = SCons.Defaults.Program
    
    env['SHLINK']      = '$LINK'
    env['SHLINKFLAGS'] = '$LINKFLAGS -shared'
    env['SHLINKCOM']   = '$SHLINK $SHLINKFLAGS -o $TARGET $SOURCES $_LIBDIRFLAGS $_LIBFLAGS'
    env['SHLIBEMITTER']= None
    env['LINK']        = env.Detect(linkers) or 'cc'
    env['LINKFLAGS']   = '-LANG:std'
    env['LINKCOM']     = '$LINK $LINKFLAGS -o $TARGET $SOURCES $_LIBDIRFLAGS $_LIBFLAGS'
    env['LIBDIRPREFIX']='-L'
    env['LIBDIRSUFFIX']=''
    env['_LIBFLAGS']='${_stripixes(LIBLINKPREFIX, LIBS, LIBLINKSUFFIX, LIBPREFIX, LIBSUFFIX, __env__)}'
    env['LIBLINKPREFIX']='-l'
    env['LIBLINKSUFFIX']=''

def exists(env):
    return env.Detect(linkers)
