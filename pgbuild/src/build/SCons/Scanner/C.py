"""SCons.Scanner.C

This module implements the depenency scanner for C/C++ code. 

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

__revision__ = "src/engine/SCons/Scanner/C.py 0.D013 2003/03/31 21:46:41 software"


import re

import SCons.Node
import SCons.Node.FS
import SCons.Scanner
import SCons.Util
import SCons.Warnings

include_re = re.compile('^[ \t]*#[ \t]*include[ \t]+(<|")([^>"]+)(>|")', re.M)

def CScan(fs = SCons.Node.FS.default_fs):
    """Return a prototype Scanner instance for scanning source files
    that use the C pre-processor"""
    cs = SCons.Scanner.Current(scan, "CScan", fs,
                               [".c", ".C", ".cxx", ".cpp", ".c++", ".cc",
                                ".h", ".H", ".hxx", ".hpp", ".hh",
                                ".F", ".fpp", ".FPP"],
                               path_function = path,
                               recursive = 1)
    return cs

def path(env, dir, fs = SCons.Node.FS.default_fs):
    try:
        cpppath = env['CPPPATH']
    except KeyError:
        return ()
    return tuple(fs.Rsearchall(SCons.Util.mapPaths(cpppath, dir, env),
                               clazz = SCons.Node.FS.Dir,
                               must_exist = 0))

def scan(node, env, cpppath = (), fs = SCons.Node.FS.default_fs):
    """
    scan(node, Environment) -> [node]

    the C/C++ dependency scanner function

    This function is intentionally simple. There are two rules it
    follows:
    
    1) #include <foo.h> - search for foo.h in CPPPATH followed by the
        directory 'filename' is in
    2) #include \"foo.h\" - search for foo.h in the directory 'filename' is
       in followed by CPPPATH

    These rules approximate the behaviour of most C/C++ compilers.

    This scanner also ignores #ifdef and other preprocessor conditionals, so
    it may find more depencies than there really are, but it never misses
    dependencies.
    """

    node = node.rfile()

    # This function caches the following information:
    # node.includes - the result of include_re.findall()

    if not node.exists():
        return []

    # cache the includes list in node so we only scan it once:
    if node.includes != None:
        includes = node.includes
    else:
        includes = include_re.findall(node.get_contents())
        node.includes = includes

    nodes = []
    source_dir = node.get_dir()
    for include in includes:
        if include[0] == '"':
            n = SCons.Node.FS.find_file(include[1],
                                        (source_dir,) + cpppath,
                                        fs.File)
        else:
            n = SCons.Node.FS.find_file(include[1],
                                        cpppath + (source_dir,),
                                        fs.File)

        if not n is None:
            nodes.append(n)
        else:
            SCons.Warnings.warn(SCons.Warnings.DependencyWarning,
                                "No dependency generated for file: %s (included from: %s) -- file not found" % (include[1], node))

    # Schwartzian transform from the Python FAQ Wizard
    def st(List, Metric):
        def pairing(element, M = Metric):
            return (M(element), element)
        def stripit(pair):
            return pair[1]
        paired = map(pairing, List)
        paired.sort()
        return map(stripit, paired)
    
    def normalize(node):
        # We don't want the order of includes to be 
        # modified by case changes on case insensitive OSes, so
        # normalize the case of the filename here:
        # (see test/win32pathmadness.py for a test of this)
        return SCons.Node.FS._my_normcase(str(node))

    return st(nodes, normalize)
