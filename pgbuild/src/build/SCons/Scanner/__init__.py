"""SCons.Scanner

The Scanner package for the SCons software construction utility.

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

__revision__ = "src/engine/SCons/Scanner/__init__.py 0.D013 2003/03/31 21:46:41 software"


import SCons.Node.FS
import SCons.Sig
import SCons.Util


class _Null:
    pass

# This is used instead of None as a default argument value so None can be
# used as an actual argument value.
_null = _Null

class Base:
    """
    The base class for dependency scanners.  This implements
    straightforward, single-pass scanning of a single file.
    """

    def __init__(self,
                 function,
                 name = "NONE",
                 argument = _null,
                 skeys = [],
                 path_function = None,
                 node_class = SCons.Node.FS.Entry,
                 node_factory = SCons.Node.FS.default_fs.File,
                 scan_check = None,
                 recursive = None):
        """
        Construct a new scanner object given a scanner function.

        'function' - a scanner function taking two or three
        arguments and returning a list of strings.

        'name' - a name for identifying this scanner object.

        'argument' - an optional argument that, if specified, will be
        passed to both the scanner function and the path_function.

        'skeys' - an optional list argument that can be used to determine
        which scanner should be used for a given Node. In the case of File
        nodes, for example, the 'skeys' would be file suffixes.

        'path_function' - a function that takes one to three arguments
        (a construction environment, optional directory, and optional
        argument for this instance) and returns a tuple of the
        directories that can be searched for implicit dependency files.

        'node_class' - the class of Nodes which this scan will return.
        If node_class is None, then this scanner will not enforce any
        Node conversion and will return the raw results from the
        underlying scanner function.

        'node_factory' - the factory function to be called to translate
        the raw results returned by the scanner function into the
        expected node_class objects.

        'scan_check' - a function to be called to first check whether
        this node really needs to be scanned.

        'recursive' - specifies that this scanner should be invoked
        recursively on the implicit dependencies it returns (the
        canonical example being #include lines in C source files).

        The scanner function's first argument will be the name of a file
        that should be scanned for dependencies, the second argument will
        be an Environment object, the third argument will be the value
        passed into 'argument', and the returned list should contain the
        Nodes for all the direct dependencies of the file.

        Examples:

        s = Scanner(my_scanner_function)

        s = Scanner(function = my_scanner_function)

        s = Scanner(function = my_scanner_function, argument = 'foo')

        """

        # Note: this class could easily work with scanner functions that take
        # something other than a filename as an argument (e.g. a database
        # node) and a dependencies list that aren't file names. All that
        # would need to be changed is the documentation.

        self.function = function
        self.path_function = path_function
        self.name = name
        self.argument = argument
        self.skeys = skeys
        self.node_class = node_class
        self.node_factory = node_factory
        self.scan_check = scan_check
        self.recursive = recursive

    def path(self, env, dir = None):
        if not self.path_function:
            return ()
        if not self.argument is _null:
            return self.path_function(env, dir, self.argument)
        else:
            return self.path_function(env, dir)

    def __call__(self, node, env, path = ()):
        """
        This method scans a single object. 'node' is the node
        that will be passed to the scanner function, and 'env' is the
        environment that will be passed to the scanner function. A list of
        direct dependency nodes for the specified node will be returned.
        """
        if self.scan_check and not self.scan_check(node):
            return []

        if not self.argument is _null:
            list = self.function(node, env, path, self.argument)
        else:
            list = self.function(node, env, path)
        kw = {}
        if hasattr(node, 'dir'):
            kw['directory'] = node.dir
        nodes = []
        for l in list:
            if self.node_class and not isinstance(l, self.node_class):
                l = apply(self.node_factory, (l,), kw)
            nodes.append(l)
        return nodes

    def __cmp__(self, other):
        return cmp(self.__dict__, other.__dict__)

    def __hash__(self):
        return hash(repr(self))

    def add_skey(self, skey):
        """Add a skey to the list of skeys"""
        self.skeys.append(skey)

class Current(Base):
    """
    A class for scanning files that are source files (have no builder)
    or are derived files and are current (which implies that they exist,
    either locally or in a repository).
    """

    def __init__(self, *args, **kw):
        def current_check(node):
            c = not node.has_builder() or node.current(SCons.Sig.default_calc)
            return c
        kw['scan_check'] = current_check
        apply(Base.__init__, (self,) + args, kw)
