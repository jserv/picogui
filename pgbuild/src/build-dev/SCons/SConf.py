"""SCons.SConf

Autoconf-like configuration support.
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

__revision__ = "src/engine/SCons/SConf.py 0.D014 2003/05/21 13:50:45 software"

import cPickle
import os
import shutil
import sys
import traceback
from types import *

import SCons.Action
import SCons.Builder
import SCons.Errors
import SCons.Node.FS
import SCons.Taskmaster
import SCons.Util
import SCons.Warnings

# First i thought of using a different filesystem as the default_fs,
# but it showed up that there are too many side effects in doing that.
SConfFS=SCons.Node.FS.default_fs

_ac_build_counter = 0
_ac_config_counter = 0
_activeSConfObjects = {}

class SConfWarning(SCons.Warnings.Warning):
    pass
SCons.Warnings.enableWarningClass( SConfWarning )

# action to create the source
def _createSource( target, source, env ):
    fd = open(target[0].get_path(), "w")
    fd.write(env['SCONF_TEXT'])
    fd.close()

def _stringSource( target, source, env ):
    import string
    return (target[0].get_path() + ' <- \n  |' +
            string.replace( env['SCONF_TEXT'], "\n", "\n  |" ) )


class SConf:
    """This is simply a class to represent a configure context. After
    creating a SConf object, you can call any tests. After finished with your
    tests, be sure to call the Finish() method, which returns the modified
    environment.
    Some words about caching: In most cases, it is not necessary to cache
    Test results explicitely. Instead, we use the scons dependency checking
    mechanism. For example, if one wants to compile a test program
    (SConf.TryLink), the compiler is only called, if the program dependencies
    have changed. However, if the program could not be compiled in a former
    SConf run, we need to explicitely cache this error.
    """

    def __init__(self, env, custom_tests = {}, conf_dir='#/.sconf_temp',
                 log_file='#/config.log'): 
        """Constructor. Pass additional tests in the custom_tests-dictinary,
        e.g. custom_tests={'CheckPrivate':MyPrivateTest}, where MyPrivateTest
        defines a custom test.
        Note also the conf_dir and log_file arguments (you may want to
        build tests in the BuildDir, not in the SourceDir)
        """
        global SConfFS
        if not SConfFS:
            SConfFS = SCons.Node.FS.FS(SCons.Node.FS.default_fs.pathTop)
        if len(_activeSConfObjects.keys()) > 0:
            raise (SCons.Errors.UserError,
                   "Only one SConf object may be active at one time")
        self.env = env
        if log_file != None:
            self.logfile = SConfFS.File(log_file)
        else:
            self.logfile = None
        self.logstream = None
        self.lastTarget = None

        # add default tests
        default_tests = {
                 'CheckCHeader'       : CheckCHeader,
                 'CheckCXXHeader'     : CheckCXXHeader,
                 'CheckLib'           : CheckLib,
                 'CheckLibWithHeader' : CheckLibWithHeader
               }
        self.AddTests(default_tests)
        self.AddTests(custom_tests)
        self.confdir = SConfFS.Dir(conf_dir)
        self.calc = None
        self.cache = {}
        self._startup()

    def Finish(self):
        """Call this method after finished with your tests:
        env = sconf.Finish()"""
        global _lastSConfObj
        _lastSConfObj = None
        self._shutdown()
        return self.env

    def _setCache(self, nodes, already_done = []):
        # Set up actions used for caching errors
        # Caching positive tests should not be necessary, cause
        # the build system knows, if test objects/programs/outputs
        # are up to date.
        for n in nodes:
            # The 'n in already_done' expression is not really efficient.
            # We may do something more sophisticated in the future :-),
            # but there should not be that many dependencies in configure
            # tests
            if (n.has_builder() and
                not n in already_done):
                n.add_pre_action(SCons.Action.Action(self._preCache,
                                                     self._stringCache))
                n.add_post_action(SCons.Action.Action(self._postCache,
                                                      self._stringCache))
                already_done.append( n )
            self._setCache(n.children())

    def BuildNodes(self, nodes):
        """
        Tries to build the given nodes immediately. Returns 1 on success,
        0 on error.
        """

        import SCons.Script    # really ugly, but we need BuildTask :-(
        # Is it better to provide a seperate Task for SConf builds ?
        class SConfBuildTask(SCons.Script.BuildTask):
            """Errors in SConf builds are not fatal, so we override
            the do_failed method"""
            def do_failed(self, status=2):
                pass

        if self.logstream != None:
            # override stdout / stderr to write in log file
            oldStdout = sys.stdout
            sys.stdout = self.logstream
            oldStderr = sys.stderr
            sys.stderr = self.logstream
        self._setCache( nodes ) 
        ret = 1

        try:
            # ToDo: use user options for calc
            self.calc = SCons.Sig.Calculator(max_drift=0)
            tm = SCons.Taskmaster.Taskmaster( nodes,
                                              SConfBuildTask,
                                              self.calc )
            # we don't want to build tests in parallel
            jobs = SCons.Job.Jobs(1, tm )
            try:
                jobs.run()
            except SCons.Errors.BuildError, e:
                sys.stderr.write("scons: *** [%s] %s\n" % (e.node, e.errstr))
                if e.errstr == 'Exception':
                    traceback.print_exception(e.args[0], e.args[1], e.args[2])
            except:
                raise

            for n in nodes:
                state = n.get_state()
                if (state != SCons.Node.executed and
                    state != SCons.Node.up_to_date):
                    # the node could not be built. we return 0 in this case
                    ret = 0
        finally:
            if self.logstream != None:
                # restore stdout / stderr
                sys.stdout = oldStdout
                sys.stderr = oldStderr
        return ret


    def TryBuild(self, builder, text = None, extension = ""):
        """Low level TryBuild implementation. Normally you don't need to
        call that - you can use TryCompile / TryLink / TryRun instead
        """
        global _ac_build_counter

        nodesToBeBuilt = []

        f = "conftest_" + str(_ac_build_counter)
        pref = self.env.subst( builder.builder.prefix )
        suff = self.env.subst( builder.builder.suffix )
        target = self.confdir.File(pref + f + suff)
        self.env['SCONF_TEXT'] = text
        if text != None:
            source = self.confdir.File(f + extension)
            sourceNode = self.env.SConfSourceBuilder(target=source,
                                                     source=None)
            nodesToBeBuilt.append(sourceNode)
        else:
            source = None

        node = builder(target = target, source = source)
        nodesToBeBuilt.append(node)
        ret = self.BuildNodes(nodesToBeBuilt)

        del self.env['SCONF_TEXT']

        _ac_build_counter = _ac_build_counter + 1
        if ret:
            self.lastTarget = node
        else:
            self.lastTarget = None

        return ret

    def TryAction(self, action, text = None, extension = ""):
        """Tries to execute the given action with optional source file
        contents <text> and optional source file extension <extension>,
        Returns the status (0 : failed, 1 : ok) and the contents of the
        output file.
        """
        builder = SCons.Builder.Builder(action=action)
        self.env.Append( BUILDERS = {'SConfActionBuilder' : builder} )
        ok = self.TryBuild(self.env.SConfActionBuilder, text, extension)
        del self.env['BUILDERS']['SConfActionBuilder']
        if ok:
            outputStr = self.lastTarget.get_contents()
            return (1, outputStr)
        return (0, "")

    def TryCompile( self, text, extension):
        """Compiles the program given in text to an env.Object, using extension
        as file extension (e.g. '.c'). Returns 1, if compilation was
        successful, 0 otherwise. The target is saved in self.lastTarget (for
        further processing).
        """
        return self.TryBuild(self.env.Object, text, extension)

    def TryLink( self, text, extension ):
        """Compiles the program given in text to an executable env.Program,
        using extension as file extension (e.g. '.c'). Returns 1, if
        compilation was successful, 0 otherwise. The target is saved in
        self.lastTarget (for further processing).
        """
        return self.TryBuild(self.env.Program, text, extension )

    def TryRun(self, text, extension ):
        """Compiles and runs the program given in text, using extension
        as file extension (e.g. '.c'). Returns (1, outputStr) on success,
        (0, '') otherwise. The target (a file containing the program's stdout)
        is saved in self.lastTarget (for further processing).
        """
        ok = self.TryLink(text, extension)
        if( ok ):
            prog = self.lastTarget
            output = SConfFS.File(prog.get_path()+'.out')
            node = self.env.Command(output, prog, "%s > $TARGET" % (prog.get_path()))
            ok = self.BuildNodes([node])
            if ok:
                outputStr = output.get_contents()
                return( 1, outputStr)
        return (0, "")

    class TestWrapper:
        """A wrapper around Tests (to ensure sanity)"""
        def __init__(self, test, sconf):
            self.test = test
            self.sconf = sconf
        def __call__(self, *args, **kw):
            if not self.sconf.active:
                raise (SCons.Errors.UserError,
                       "Test called after sconf.Finish()")
            context = CheckContext(self.sconf)
            ret = apply(self.test, (context,) +  args, kw)
            context.Result("error: no result")
            return ret

    def AddTest(self, test_name, test_instance):
        """Adds test_class to this SConf instance. It can be called with
        self.test_name(...)"""
        setattr(self, test_name, SConf.TestWrapper(test_instance, self))

    def AddTests(self, tests):
        """Adds all the tests given in the tests dictionary to this SConf
        instance
        """
        for name in tests.keys():
            self.AddTest(name, tests[name])

    def _preCache(self, target, source, env):
        # Action before target is actually built
        #
        # We record errors in the cache. Only non-exisiting targets may
        # have recorded errors
        needs_rebuild = target[0].exists()
        buildSig = target[0].calc_signature(self.calc)
        for node in source:
            if node.get_state() != SCons.Node.up_to_date:
                # if any of the sources has changed, we cannot use our cache
                needs_rebuild = 1
        if not self.cache.has_key( target[0].get_path() ):
            # We have no recorded error, so we try to build the target
            needs_rebuild = 1
        else:
            lastBuildSig = self.cache[target[0].get_path()]['builder']
            if lastBuildSig != buildSig:
                needs_rebuild = 1
        if not needs_rebuild:
            # When we are here, we can savely pass the recorded error
            print ('(cached): Building "%s" failed in a previous run.' %
                   target[0].get_path())
            return 1
        else:
            # Otherwise, we try to record an error
            self.cache[target[0].get_path()] = {
               'builder' :  buildSig
            }

    def _postCache(self, target, source, env):
        # Action after target is successfully built
        #
        # No error during build -> remove the recorded error
        del self.cache[target[0].get_path()]

    def _stringCache(self, target, source, env):
        return None

    def _loadCache(self):
        # try to load build-error cache
        try:
            cacheDesc = cPickle.load(open(self.confdir.File(".cache").get_path()))
            if cacheDesc['scons_version'] != SCons.__version__:
                raise Exception, "version mismatch"
            self.cache = cacheDesc['data']
        except:
            self.cache = {}

    def _dumpCache(self):
        # try to dump build-error cache
        try:
            cacheDesc = {'scons_version' : SCons.__version__,
                         'data'          : self.cache }
            cPickle.dump(cacheDesc, open(self.confdir.File(".cache").get_path(),"w"))
        except Exception, e:
            # this is most likely not only an IO error, but an error
            # inside SConf ...
            SCons.Warnings.warn( SConfWarning, "Couldn't dump SConf cache" )

    def _createDir( self, node ):
        dirName = node.get_path()
        if not os.path.isdir( dirName ):
            os.makedirs( dirName )
            node._exists = 1

    def _startup(self):
        """Private method. Set up logstream, and set the environment
        variables necessary for a piped build
        """
        global _ac_config_counter
        global _activeSConfObjects
        global SConfFS
        
        self.lastEnvFs = self.env.fs
        self.env.fs = SConfFS
        self._createDir(self.confdir)
        self.confdir.up().add_ignore( [self.confdir] )

        if self.logfile != None:
            # truncate logfile, if SConf.Configure is called for the first time
            # in a build
            if _ac_config_counter == 0:
                log_mode = "w"
            else:
                log_mode = "a"
            self.logstream = open(self.logfile.get_path(), log_mode)
            # logfile may stay in a build directory, so we tell
            # the build system not to override it with a eventually
            # existing file with the same name in the source directory
            self.logfile.dir.add_ignore( [self.logfile] )
            self.env['PIPE_BUILD'] = 1
            self.env['PSTDOUT'] = self.logstream
            self.env['PSTDERR'] = self.logstream

            tb = traceback.extract_stack()[-3]
            
            self.logstream.write( '\nfile %s,line %d:\n\tConfigure( confdir = %s )\n\n' %
                                  (tb[0], tb[1], self.confdir.get_path()) )
        else: 
            self.logstream = None
        # we use a special builder to create source files from TEXT
        action = SCons.Action.Action(_createSource,
                                     _stringSource,
                                     varlist=['SCONF_TEXT'])
        sconfSrcBld = SCons.Builder.Builder(action=action)
        self.env.Append( BUILDERS={'SConfSourceBuilder':sconfSrcBld} )
        self.active = 1
        # only one SConf instance should be active at a time ...
        _activeSConfObjects[self] = None
        _ac_config_counter = _ac_config_counter + 1
        self._loadCache()

    def _shutdown(self):
        """Private method. Reset to non-piped spawn"""
        global _activeSConfObjets

        if not self.active:
            raise SCons.Errors.UserError, "Finish may be called only once!"
        if self.logstream != None:
            self.logstream.close()
            self.logstream = None
            # clean up environment
            del self.env['PIPE_BUILD']
            del self.env['PSTDOUT']
            del self.env['PSTDERR']
        # remove the SConfSourceBuilder from the environment
        blds = self.env['BUILDERS']
        del blds['SConfSourceBuilder']
        self.env.Replace( BUILDERS=blds )
        self.active = 0
        del _activeSConfObjects[self]
        self._dumpCache()
        self.env.fs = self.lastEnvFs

class CheckContext:
    """Provides a context for configure tests. Defines how a test writes to the
    screen and log file.

    A typical test is just a callable with an instance of CheckContext as
    first argument:

    def CheckCustom(context, ...)
    context.Message('Checking my weird test ... ')
    ret = myWeirdTestFunction(...)
    context.Result(ret)

    Often, myWeirdTestFunction will be one of
    context.TryCompile/context.TryLink/context.TryRun. The results of
    those are cached, for they are only rebuild, if the dependencies have
    changed.
    """

    def __init__(self, sconf):
        """Constructor. Pass the corresponding SConf instance."""
        self.sconf = sconf
        self.cached = 0
        self.show_result = 0

    def Message(self, text):
        """Inform about what we are doing right now, e.g.
        'Checking for SOMETHING ... '
        """
        # write to config.log
        if self.sconf.logstream != None:
            self.sconf.logstream.write(text + '\n')
        sys.stdout.write(text)
        self.show_result = 0

    def Result(self, res ):
        """Inform about the result of the test. res may be an integer or a
        string. In case of an integer, the written text will be 'ok' or
        'failed'.
        """
        if( type(res) == IntType ):
            if res:
                text = "ok"
            else:
                text = "failed"
        elif( type(res) == StringType ):
            text = res
        else:
            raise TypeError, "Expected string or int"
        if( self.cached ):
            text = text + " (cached)"
        if self.show_result == 0:
            if self.sconf.logstream != None:
                self.sconf.logstream.write("Result: " + text + "\n\n")
            sys.stdout.write(text + "\n")
            self.show_result = 1


    def TryBuild(self, *args, **kw):
        return apply(self.sconf.TryBuild, args, kw)

    def TryAction(self, *args, **kw):
        return apply(self.sconf.TryAction, args, kw)

    def TryCompile(self, *args, **kw):
        return apply(self.sconf.TryCompile, args, kw)

    def TryLink(self, *args, **kw):
        return apply(self.sconf.TryLink, args, kw)

    def TryRun(self, *args, **kw):
        return apply(self.sconf.TryRun, args, kw)

    def __getattr__( self, attr ):
        if( attr == 'env' ):
            return self.sconf.env
        elif( attr == 'lastTarget' ):
            return self.sconf.lastTarget
        else:
            raise AttributeError, "CheckContext instance has no attribute '%s'" % attr

def _header_prog( header, include_quotes ):
    return "#include %s%s%s\n\n" % (include_quotes[0],
                                    header,
                                    include_quotes[1])

def CheckCHeader(test, header, include_quotes='""'):
    """
    A test for a c header file.
    """
    # ToDo: Support also system header files (i.e. #include <header.h>)
    test.Message("Checking for C header %s ... " % header)
    ret = test.TryCompile(_header_prog(header, include_quotes), ".c")
    test.Result( ret )
    return ret


def CheckCXXHeader(test, header, include_quotes='""'):
    """
    A test for a c++ header file.
    """
    # ToDo: Support also system header files (i.e. #include <header.h>)
    test.Message("Checking for C++ header %s ... " % header)
    ret = test.TryCompile(_header_prog(header, include_quotes), ".cpp")
    test.Result( ret )
    return ret

def CheckLib(test, library=None, symbol="main", autoadd=1):
    """
    A test for a library. See also CheckLibWithHeader.
    Note that library may also be None to test whether the given symbol
    compiles without flags.
    """
    # ToDo: accept path for the library
    test.Message("Checking for %s in library %s ... " % (symbol, library))
    oldLIBS = test.env.get( 'LIBS', [] )

    # NOTE: we allow this at in the case that we don't know what the
    # library is called like when we get --libs from a configure script
    if library != None:
        test.env.Append(LIBS = [ library ])

    text = ""
    if symbol != "main":
        text = text + """
#ifdef __cplusplus
extern "C"
#endif
char %s();""" % symbol
    text = text + """
int
main() {
%s();
return 0;
}
\n\n""" % symbol

    ret = test.TryLink( text, ".c" )
    if not autoadd or not ret:
        test.env.Replace(LIBS=oldLIBS)

    test.Result(ret)
    return ret

def CheckLibWithHeader(test, library, header, language, call="main();", autoadd=1):
    # ToDo: accept path for library. Support system header files.
    """
    Another (more sophisticated) test for a library.
    Checks, if library and header is available for language (maybe 'C'
    or 'CXX'). Call maybe be a valid expression _with_ a trailing ';'.
    As in CheckLib, we support library=None, to test if the call compiles
    without extra link flags.
    """
    test.Message("Checking for %s in library %s (header %s) ... " %
                 (call, library, header))
    oldLIBS= test.env.get( 'LIBS', [] )

    # NOTE: we allow this at in the case that we don't know what the
    # library is called like when we get --libs from a configure script
    if library != None:
        test.env.Append(LIBS = [ library ])

    text  = """\
#include "%s"
int main() {
  %s
  return 0;
}
""" % (header, call)

    if language in ["C", "c"]:
        extension=".c"
    elif language in ["CXX", "cxx", "C++", "c++"]:
        extension=".cpp"
    else:
        raise SCons.Errors.UserError, "Unknown language!"

    ret = test.TryLink( text, extension)
    if not autoadd or not ret:
        test.env.Replace( LIBS = oldLIBS )

    test.Result(ret)
    return ret
