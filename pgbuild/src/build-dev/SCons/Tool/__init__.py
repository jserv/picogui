"""SCons.Tool

SCons tool selection.

This looks for modules that define a callable object that can modify
a construction environment as appropriate for a given tool (or tool
chain).

Note that because this subsysem just *selects* a callable that can
modify a construction environment, it's possible for people to define
their own "tool specification" in an arbitrary callable function.  No
one needs to use or tie in to this subsystem in order to roll their own
tool definition.
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

__revision__ = "src/engine/SCons/Tool/__init__.py 0.D014 2003/05/21 13:50:45 software"

import imp
import sys

import SCons.Errors
import SCons.Defaults

class ToolSpec:
    def __init__(self, name):
        self.name = name

    def __call__(self, env, *args, **kw):
        env.Append(TOOLS = [ self.name ])
        apply(self.generate, ( env, ) + args, kw)

    def __str__(self):
        return self.name
    
def Tool(name):
    """Select a canned Tool specification.
    """
    full_name = 'SCons.Tool.' + name
    if not sys.modules.has_key(full_name):
        try:
            file, path, desc = imp.find_module(name,
                                        sys.modules['SCons.Tool'].__path__)
            mod = imp.load_module(full_name, file, path, desc)
            setattr(SCons.Tool, name, mod)
        except ImportError:
            raise SCons.Errors.UserError, "No tool named '%s'" % name
        if file:
            file.close()
    spec = ToolSpec(name)
    spec.generate = sys.modules[full_name].generate
    spec.exists = sys.modules[full_name].exists
    return spec

def createObjBuilders(env):
    """This is a utility function that creates the Object
    and SharedObject Builders in an Environment if they
    are not there already.

    If they are there already, we return the existing ones.

    This is a separate function because soooo many Tools
    use this functionality.

    The return is a 2-tuple of (StaticObject, SharedObject)
    """

    try:
        static_obj = env['BUILDERS']['Object']
    except KeyError:
        static_obj = SCons.Defaults.StaticObject()
        env['BUILDERS']['Object'] = static_obj
        env['BUILDERS']['StaticObject'] = static_obj

    try:
        shared_obj = env['BUILDERS']['SharedObject']
    except KeyError:
        shared_obj = SCons.Defaults.SharedObject()
        env['BUILDERS']['SharedObject'] = shared_obj

    return (static_obj, shared_obj)

def createCFileBuilders(env):
    """This is a utility function that creates the CFile/CXXFile
    Builders in an Environment if they
    are not there already.

    If they are there already, we return the existing ones.

    This is a separate function because soooo many Tools
    use this functionality.

    The return is a 2-tuple of (CFile, CXXFile)
    """

    try:
        c_file = env['BUILDERS']['CFile']
    except KeyError:
        c_file = SCons.Defaults.CFile()
        env['BUILDERS']['CFile'] = c_file
        env['CFILESUFFIX'] = '.c'

    try:
        cxx_file = env['BUILDERS']['CXXFile']
    except KeyError:
        cxx_file = SCons.Defaults.CXXFile()
        env['BUILDERS']['CXXFile'] = cxx_file
        env['CXXFILESUFFIX'] = '.cc'

    return (c_file, cxx_file)

def FindTool(tools, env):
    for tool in tools:
        t = Tool(tool)
        if t.exists(env):
            return tool
    return None

def FindAllTools(tools, env):
    def ToolExists(tool, env=env):
        return Tool(tool).exists(env)
    return filter (ToolExists, tools)
             
def tool_list(platform, env):

    # XXX this logic about what tool to prefer on which platform
    #     should be moved into either the platform files or
    #     the tool files themselves.
    # The search orders here are described in the man page.  If you
    # change these search orders, update the man page as well.
    if str(platform) == 'win32':
        "prefer Microsoft tools on Windows"
        linkers = ['mslink', 'gnulink', 'ilink', 'linkloc' ]
        c_compilers = ['msvc', 'mingw', 'gcc', 'icc' ]
        assemblers = ['masm', 'nasm', 'gas', '386asm' ]
        fortran_compilers = ['g77', 'ifl']
        ars = ['mslib', 'ar']
    elif str(platform) == 'os2':
        "prefer IBM tools on OS/2"
        linkers = ['ilink', 'gnulink', 'mslink']
        c_compilers = ['icc', 'gcc', 'msvc']
        assemblers = ['nasm', 'masm', 'gas']
        fortran_compilers = ['ifl', 'g77']
        ars = ['ar', 'mslib']
    elif str(platform) == 'irix':
        "prefer MIPSPro on IRIX"
        linkers = ['sgilink', 'gnulink']
        c_compilers = ['sgicc', 'gcc']
        assemblers = ['as', 'gas']
        fortran_compilers = ['f77', 'g77']
        ars = ['sgiar']
    elif str(platform) == 'sunos':
        "prefer Forte tools on SunOS"
        linkers = ['sunlink', 'gnulink']
        c_compilers = ['suncc', 'gcc']
        assemblers = ['as', 'gas']
        fortran_compilers = ['f77', 'g77']
        ars = ['sunar']
    elif str(platform) == 'hpux':
        "prefer aCC tools on HP-UX"
        linkers = ['hplink', 'gnulink']
        c_compilers = ['hpcc', 'gcc']
        assemblers = ['as', 'gas']
        fortran_compilers = ['f77', 'g77']
        ars = ['ar']
    elif str(platform) == 'aix':
        "prefer AIX Visual Age tools on AIX"
        linkers = ['aixlink', 'gnulink']
        c_compilers = ['aixcc', 'gcc']
        assemblers = ['as', 'gas']
        fortran_compilers = ['aixf77', 'g77']
        ars = ['ar']
    else:
        "prefer GNU tools on all other platforms"
        linkers = ['gnulink', 'mslink', 'ilink']
        c_compilers = ['gcc', 'msvc', 'icc']
        assemblers = ['gas', 'nasm', 'masm']
        fortran_compilers = ['g77', 'ifl']
        ars = ['ar', 'mslib']

    c_compiler = FindTool(c_compilers, env) or c_compilers[0]
 
    # XXX this logic about what tool provides what should somehow be
    #     moved into the tool files themselves.
    if c_compiler and c_compiler == 'mingw':
        # MinGW contains a linker, C compiler, C++ compiler, 
        # Fortran compiler, archiver and assember:
        linker = None
        assembler = None
        fortran_compiler = None
        ar = None
        cxx_compiler = None
    else:
        linker = FindTool(linkers, env) or linkers[0]
        assembler = FindTool(assemblers, env) or assemblers[0]
        fortran_compiler = FindTool(fortran_compilers, env) or fortran_compilers[0]
        ar = FindTool(ars, env) or ars[0]

        # Don't use g++ if the C compiler has built-in C++ support:
        if c_compiler and (c_compiler == 'msvc' or c_compiler == 'icc' or
                           c_compiler == 'sgicc'):
            cxx_compiler = None
        else:
            cxx_compiler = FindTool(['g++'], env)
        
    other_tools = FindAllTools(['BitKeeper', 'CVS',
                                'dvipdf', 'dvips', 'gs',
                                'jar', 'javac', 'javah',
                                'latex', 'lex', 'midl',
                                'pdflatex', 'pdftex', 'Perforce',
                                'RCS', 'rmic', 'SCCS',
                                # 'Subversion',
                                'tar', 'tex', 'yacc', 'zip'],
                               env)

    tools = ([linker, c_compiler, cxx_compiler,
              fortran_compiler, assembler, ar]
             + other_tools)
    
    return filter(lambda x: x, tools)
