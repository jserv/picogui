"""SCons.Tool.zip

Tool-specific initialization for zip.

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

__revision__ = "src/engine/SCons/Tool/zip.py 0.D014 2003/05/21 13:50:45 software"

import os.path

import SCons.Builder
import SCons.Node.FS
import SCons.Util

try:
    import zipfile

    def zip(target, source, env):
        def visit(arg, dirname, names):
            for name in names:
                path = os.path.join(dirname, name)
                if os.path.isfile(path):
                    arg.write(path)
        zf = zipfile.ZipFile(str(target[0]), 'w')
        for s in source:
            if os.path.isdir(str(s)):
                os.path.walk(str(s), visit, zf)
            else:
                zf.write(str(s))
        zf.close()

    internal_zip = 1

except ImportError:
    zip = "$ZIP $ZIPFLAGS ${TARGET.abspath} $SOURCES"

    internal_zip = 0

zipAction = SCons.Action.Action(zip)

ZipBuilder = SCons.Builder.Builder(action = '$ZIPCOM',
                                   source_factory = SCons.Node.FS.default_fs.Entry,
                                   suffix = '$ZIPSUFFIX',
                                   multi = 1)


def generate(env):
    """Add Builders and construction variables for zip to an Environment."""
    try:
        bld = env['BUILDERS']['Zip']
    except KeyError:
        bld = ZipBuilder
        env['BUILDERS']['Zip'] = bld

    env['ZIP']        = 'zip'
    env['ZIPFLAGS']   = ''
    env['ZIPCOM']     = zipAction
    env['ZIPSUFFIX']  = '.zip'

def exists(env):
    return internal_zip or env.Detect('zip')
