"""SCons.Tool.mslink

Tool-specific initialization for the Microsoft linker.

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

__revision__ = "src/engine/SCons/Tool/mslink.py 0.D013 2003/03/31 21:46:41 software"

import os.path
import string

import SCons.Action
import SCons.Defaults
import SCons.Errors
import SCons.Util
import msvc

from SCons.Tool.msvc import get_msdev_paths

def pdbGenerator(env, target, source):
    if target and env.has_key('PDB') and env['PDB']:
        return ['/PDB:%s'%target[0].File(env['PDB']), '/DEBUG']

def win32ShlinkTargets(target, source, env):
    if target:
        listCmd = []
        dll = env.FindIxes(target, 'SHLIBPREFIX', 'SHLIBSUFFIX')
        if dll: listCmd.append("/out:%s"%dll)

        implib = env.FindIxes(target, 'LIBPREFIX', 'LIBSUFFIX')
        if implib: listCmd.append("/implib:%s"%implib)

        return listCmd
    else:
        # For signature calculation
        return '/out:$TARGET'

def win32ShlinkSources(target, source, env):
    if target:
        listCmd = []

        deffile = env.FindIxes(source, "WIN32DEFPREFIX", "WIN32DEFSUFFIX")
        for src in source:
            if src == deffile:
                # Treat this source as a .def file.
                listCmd.append("/def:%s" % src)
            else:
                # Just treat it as a generic source file.
                listCmd.append(str(src))
        return listCmd
    else:
        # For signature calculation
        return "$SOURCES"

def win32LibEmitter(target, source, env):
    msvc.validate_vars(env)
    
    dll = env.FindIxes(target, "SHLIBPREFIX", "SHLIBSUFFIX")
    no_import_lib = env.get('no_import_lib', 0)
    
    if not dll:
        raise SCons.Errors.UserError, "A shared library should have exactly one target with the suffix: %s" % env.subst("$SHLIBSUFFIX")

    if env.get("WIN32_INSERT_DEF", 0) and \
       not env.FindIxes(source, "WIN32DEFPREFIX", "WIN32DEFSUFFIX"):

        # append a def file to the list of sources
        source.append(env.ReplaceIxes(dll, 
                                      "SHLIBPREFIX", "SHLIBSUFFIX",
                                      "WIN32DEFPREFIX", "WIN32DEFSUFFIX"))

    if env.has_key('PDB') and env['PDB']:
        env.SideEffect(env['PDB'], target)
        env.Precious(env['PDB'])

    if not no_import_lib and \
       not env.FindIxes(target, "LIBPREFIX", "LIBSUFFIX"):
        # Append an import library to the list of targets.
        target.append(env.ReplaceIxes(dll, 
                                      "SHLIBPREFIX", "SHLIBSUFFIX",
                                      "LIBPREFIX", "LIBSUFFIX"))

    return (target, source)

def prog_emitter(target, source, env):
    msvc.validate_vars(env)
    
    if env.has_key('PDB') and env['PDB']:
        env.SideEffect(env['PDB'], target)
        env.Precious(env['PDB'])
        
    return (target,source)

def generate(env, platform):
    """Add Builders and construction variables for ar to an Environment."""
    env['BUILDERS']['SharedLibrary'] = SCons.Defaults.SharedLibrary
    env['BUILDERS']['Program'] = SCons.Defaults.Program
    
    env['SHLINK']      = '$LINK'
    env['SHLINKFLAGS'] = '$LINKFLAGS /dll'
    env['_SHLINK_TARGETS'] = win32ShlinkTargets
    env['_SHLINK_SOURCES'] = win32ShlinkSources
    env['SHLINKCOM']   = '${TEMPFILE("$SHLINK $SHLINKFLAGS $_SHLINK_TARGETS $( $_LIBDIRFLAGS $) $_LIBFLAGS $_PDB $_SHLINK_SOURCES")}'
    env['SHLIBEMITTER']= win32LibEmitter
    env['LINK']        = 'link'
    env['LINKFLAGS']   = '/nologo'
    env['_PDB'] = pdbGenerator
    env['LINKCOM'] = '${TEMPFILE("$LINK $LINKFLAGS /OUT:$TARGET $( $_LIBDIRFLAGS $) $_LIBFLAGS $_PDB $SOURCES")}'
    env['PROGEMITTER'] = prog_emitter
    env['LIBDIRPREFIX']='/LIBPATH:'
    env['LIBDIRSUFFIX']=''
    env['LIBLINKPREFIX']=''
    env['LIBLINKSUFFIX']='$LIBSUFFIX'

    env['WIN32DEFPREFIX']        = ''
    env['WIN32DEFSUFFIX']        = '.def'
    env['WIN32_INSERT_DEF']      = 0

    if SCons.Util.can_read_reg:
        include_path, lib_path, exe_path = get_msdev_paths()
        env['ENV']['LIB']            = lib_path
        env['ENV']['PATH']           = exe_path

def exists(env):
    return env.Detect('link')
