"""SCons.Tool.RCS.py

Tool-specific initialization for RCS.

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

__revision__ = "src/engine/SCons/Tool/RCS.py 0.D013 2003/03/31 21:46:41 software"

import SCons.Builder

def generate(env, platform):
    """Add a Builder factory function and construction variables for
    RCS to an Environment."""

    def RCSFactory(env=env):
        """ """
        return SCons.Builder.Builder(action = '$RCS_COCOM', env = env)

    setattr(env, 'RCS', RCSFactory)

    env['RCS']          = 'rcs'
    env['RCS_CO']       = 'co'
    env['RCS_COFLAGS']  = ''
    env['RCS_COCOM']    = '$RCS_CO $RCS_COFLAGS $TARGET'

def exists(env):
    return env.Detect('rcs')