"""SCons.Taskmaster

Generic Taskmaster.

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

__revision__ = "src/engine/SCons/Taskmaster.py 0.D014 2003/05/21 13:50:45 software"

import string
import sys

import SCons.Node
import SCons.Errors

class Task:
    """Default SCons build engine task.

    This controls the interaction of the actual building of node
    and the rest of the engine.

    This is expected to handle all of the normally-customizable
    aspects of controlling a build, so any given application
    *should* be able to do what it wants by sub-classing this
    class and overriding methods as appropriate.  If an application
    needs to customze something by sub-classing Taskmaster (or
    some other build engine class), we should first try to migrate
    that functionality into this class.

    Note that it's generally a good idea for sub-classes to call
    these methods explicitly to update state, etc., rather than
    roll their own interaction with Taskmaster from scratch."""
    def __init__(self, tm, targets, top, node):
        self.tm = tm
        self.targets = targets
        self.top = top
        self.node = node

    def display(self, message):
        """Allow the calling interface to display a message
        """
        pass

    def prepare(self):
        """Called just before the task is executed.

        This unlinks all targets and makes all directories before
        building anything."""

        # Now that it's the appropriate time, give the TaskMaster a
        # chance to raise any exceptions it encountered while preparing
        # this task.
        self.tm.exception_raise()

        if self.tm.message:
            self.display(self.tm.message)
            self.tm.message = None

        for t in self.targets:
            t.prepare()

    def execute(self):
        """Called to execute the task.

        This method is called from multiple threads in a parallel build,
        so only do thread safe stuff here.  Do thread unsafe stuff in
        prepare(), executed() or failed()."""

        try:
            self.targets[0].build()
        except KeyboardInterrupt:
            raise
        except SystemExit:
            raise SCons.Errors.ExplicitExit(self.targets[0], sys.exc_value.code)
        except SCons.Errors.UserError:
            raise
        except SCons.Errors.BuildError:
            raise
        except:
            raise SCons.Errors.BuildError(self.targets[0],
                                          "Exception",
                                          sys.exc_type,
                                          sys.exc_value,
                                          sys.exc_traceback)

    def get_target(self):
        """Fetch the target being built or updated by this task.
        """
        return self.node

    def executed(self):
        """Called when the task has been successfully executed.

        This may have been a do-nothing operation (to preserve
        build order), so check the node's state before updating
        things.  Most importantly, this calls back to the
        Taskmaster to put any node tasks waiting on this one
        back on the pending list."""

        if self.targets[0].get_state() == SCons.Node.executing:
            for t in self.targets:
                for side_effect in t.side_effects:
                    side_effect.set_state(None)
                t.set_state(SCons.Node.executed)
                t.built()
        else:
            for t in self.targets:
                t.visited()

        self.tm.executed(self.node)

    def failed(self):
        """Default action when a task fails:  stop the build."""
        self.fail_stop()

    def fail_stop(self):
        """Explicit stop-the-build failure."""
        for t in self.targets:
            t.set_state(SCons.Node.failed)
        self.tm.stop()

    def fail_continue(self):
        """Explicit continue-the-build failure.

        This sets failure status on the target nodes and all of
        their dependent parent nodes.
        """
        for t in self.targets:
            def get_parents(node, parent): return node.get_parents()
            def set_state(node, parent): node.set_state(SCons.Node.failed)
            walker = SCons.Node.Walker(t, get_parents, eval_func=set_state)
            n = walker.next()
            while n:
                n = walker.next()

        self.tm.executed(self.node)

    def make_ready(self):
        """Make a task ready for execution."""
        state = SCons.Node.up_to_date
        for t in self.targets:
            if not t.current(self.tm.calc):
                state = SCons.Node.executing
        for t in self.targets:
            if state == SCons.Node.executing:
                for side_effect in t.side_effects:
                    side_effect.set_state(state)
            t.set_state(state)

def order(dependencies):
    """Re-order a list of dependencies (if we need to)."""
    return dependencies

class Calc:
    def bsig(self, node):
        """
        """
        return None

    def current(self, node, sig):
        """Default SCons build engine is-it-current function.

        This returns "always out of date," so every node is always
        built/visited.
        """
        return 0

class Taskmaster:
    """A generic Taskmaster for handling a bunch of targets.

    Classes that override methods of this class should call
    the base class method, so this class can do its thing.
    """

    def __init__(self, targets=[], tasker=Task, calc=Calc(), order=order):
        self.targets = targets # top level targets
        self.candidates = targets[:] # nodes that might be ready to be executed
        self.candidates.reverse()
        self.executing = [] # nodes that are currently executing
        self.pending = [] # nodes that depend on a currently executing node
        self.tasker = tasker
        self.ready = None # the next task that is ready to be executed
        self.calc = calc
        self.order = order
        self.exception_set(None, None)
        self.message = None

    def _find_next_ready_node(self):
        """Find the next node that is ready to be built"""

        if self.ready:
            return

        while self.candidates:
            node = self.candidates[-1]
            state = node.get_state()

            # Skip this node if it has already been executed:
            if state != None and state != SCons.Node.stack:
                self.candidates.pop()
                continue

            # Mark this node as being on the execution stack:
            node.set_state(SCons.Node.stack)

            try:
                children = node.children()
            except SystemExit:
                e = SCons.Errors.ExplicitExit(node, sys.exc_value.code)
                self.exception_set(SCons.Errors.ExplicitExit, e)
                self.candidates.pop()
                self.ready = node
                break
            except:
                # We had a problem just trying to figure out the
                # children (like a child couldn't be linked in to a
                # BuildDir, or a Scanner threw something).  Arrange to
                # raise the exception when the Task is "executed."
                self.exception_set(sys.exc_type, sys.exc_value)
                self.candidates.pop()
                self.ready = node
                break

            # Detect dependency cycles:
            def in_stack(node): return node.get_state() == SCons.Node.stack
            cycle = filter(in_stack, children)
            if cycle:
                nodes = filter(in_stack, self.candidates) + cycle
                nodes.reverse()
                desc = "Dependency cycle: " + string.join(map(str, nodes), " -> ")
                raise SCons.Errors.UserError, desc

            # Find all of the derived dependencies (that is,
            # children who have builders or are side effects):
            try:
                def derived_nodes(node): return node.is_derived() or node.is_pseudo_derived()
                derived = filter(derived_nodes, children)
            except:
                # We had a problem just trying to figure out if any of
                # the kids are derived (like a child couldn't be linked
                # from a repository).  Arrange to raise the exception
                # when the Task is "executed."
                self.exception_set(sys.exc_type, sys.exc_value)
                self.candidates.pop()
                self.ready = node
                break

            # If there aren't any children with builders and this
            # was a top-level argument, then see if we can find any
            # corresponding targets in linked build directories:
            if not derived and node in self.targets:
                alt, message = node.alter_targets()
                if alt:
                    self.message = message
                    self.candidates.pop()
                    self.candidates.extend(alt)
                    continue

            # Add derived files that have not been built
            # to the candidates list:
            def unbuilt_nodes(node): return node.get_state() == None
            not_built = filter(unbuilt_nodes, derived)
            if not_built:
                not_built.reverse()
                self.candidates.extend(self.order(not_built))
                continue

            # Skip this node if it has side-effects that are
            # currently being built:
            cont = 0
            for side_effect in node.side_effects:
                if side_effect.get_state() == SCons.Node.executing:
                    self.pending.append(node)
                    node.set_state(SCons.Node.pending)
                    self.candidates.pop()
                    cont = 1
                    break
            if cont: continue

            # Skip this node if it is pending on a currently
            # executing node:
            if node.depends_on(self.executing) or node.depends_on(self.pending):
                self.pending.append(node)
                node.set_state(SCons.Node.pending)
                self.candidates.pop()
                continue

            # The default when we've gotten through all of the checks above:
            # this node is ready to be built.
            self.candidates.pop()
            self.ready = node
            break

    def next_task(self):
        """Return the next task to be executed."""

        self._find_next_ready_node()

        node = self.ready

        if node is None:
            return None

        try:
            tlist = node.builder.targets(node)
        except AttributeError:
            tlist = [node]
        self.executing.extend(tlist)
        self.executing.extend(node.side_effects)
        
        task = self.tasker(self, tlist, node in self.targets, node)
        try:
            task.make_ready()
        except:
            # We had a problem just trying to get this task ready (like
            # a child couldn't be linked in to a BuildDir when deciding
            # whether this node is current).  Arrange to raise the
            # exception when the Task is "executed."
            self.exception_set(sys.exc_type, sys.exc_value)
        self.ready = None

        return task

    def is_blocked(self):
        self._find_next_ready_node()

        return not self.ready and self.pending

    def stop(self):
        """Stop the current build completely."""
        self.candidates = []
        self.ready = None
        self.pending = []

    def executed(self, node):
        try:
            tlist = node.builder.targets(node)
        except AttributeError:
            tlist = [node]
        for t in tlist:
            self.executing.remove(t)
        for side_effect in node.side_effects:
            self.executing.remove(side_effect)

        # move the current pending nodes to the candidates list:
        # (they may not all be ready to build, but _find_next_ready_node()
        #  will figure out which ones are really ready)
        for node in self.pending:
            node.set_state(None)
        self.pending.reverse()
        self.candidates.extend(self.pending)
        self.pending = []

    def exception_set(self, type, value):
        """Record an exception type and value to raise later, at an
        appropriate time."""
        self.exc_type = type
        self.exc_value = value

    def exception_raise(self):
        """Raise any pending exception that was recorded while
        getting a Task ready for execution."""
        if self.exc_type:
            try:
                raise self.exc_type, self.exc_value
            finally:
                self.exception_set(None, None)

