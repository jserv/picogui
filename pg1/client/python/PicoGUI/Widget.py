# Widget class

default_relationship = 'after'

import constants, struct, types

class BoundCommandMethod(object):
    def __init__(self, widget, name, ns, args):
        self.widget = widget
        self.name = name
        self.ns = ns
        self.args = args

    def __call__(self, *args):
        args = self.args + args
        self.widget.command(self.name, *args)

    def __getattr__(self, name):
        try:
            ns = self.ns[name][1]
        except KeyError:
            raise AttributeError(name)
        return BoundCommandMethod(self.widget, self.name, ns, self.args + (name,))

class CommandMethod(object):
    def __init__(self, name):
        self.name = name
        ns = constants.cmd_ns(name)
        if type(ns) is types.DictType:
            self.ns = ns
        else:
            self.ns = {}

    def __get__(self, widget, widgetclass):
        return BoundCommandMethod(widget, self.name, self.ns, ())

class WidgetProperty(object):
    def __init__(self, name):
        self.name = name

    def __get__(self, widget, widgetclass):
        result = widget.server.get(widget.handle, self.name)
        ns = constants.prop_ns(self.name)
        return constants.unresolve(result, ns, widget.server)

    def __set__(self, widget, value):
        widget.server.set(widget.handle, self.name, value)

    def __delete__(self, widget):
        raise AttributeError, 'cannot delete a widget property'

class Widget(object):
    def __init__(self, server, handle, parent=None, type=None):
        self.server = server
        self.handle = handle
        self.default_relationship = default_relationship
        self.parent = parent
        try:
            self.app = parent.app
        except AttributeError:
            self.app = None
        self._type = type

    def __eq__(self, other):
        return (self.server is other.server) and (self.handle == other.handle)

    def _notify_new_widget(self, new):
        # do nothing, but this is overridden in Application to maintain a registry
        if self.parent and hasattr(self.parent, '_notify_new_widget'):
            self.parent._notify_new_widget(new)

    def _get_widget_factory(self, wtype, factory):
        if callable(wtype):
            factory = wtype
            # don't use getattr() so that it isn't inherited
            if getattr(factory, '__dict__', {}).has_key('widget_type'):
                name = factory.widget_type
            else:
                name = factory.__name__
        else:
            name = wtype
        if factory is None:
            factory = Widget
        return name, factory

    def addWidget(self, wtype, relationship=None, wrapper_class=None):
        name, factory = self._get_widget_factory(wtype, wrapper_class)
        new_id = self.server.mkWidget(relationship or self.default_relationship, name, self.handle)
        new = factory(self.server, new_id, self, type=name)
        self._notify_new_widget(new)
        return new

    def detach(self):
        self.attach(0)

    def attach(self, parent, relationship=None):
        try:
            parent = parent.handle
        except AttributeError:
            pass
        self.server.attachwidget(parent, self.handle, relationship or self.default_relationship)

    def find(self, name):
        # FIXME: This should find only widgets below this one in the hierarchy.
        #        This can be implemented here pending a protocol change in pgserver.
        return Widget(self.server,self.server.findwidget(name),self.parent)

    # This is write, so a widget can be used like a file object
    # (even tough this doesn't always work... when it does it's useful)
    def write(self, data):
        self.server.writedata(self.handle,data)

    def command(self, command, *parameters):
        self.server.writecmd(self.handle, command, *parameters)

    def focus(self):
        self.server.focus(self.handle)

for pname in constants.propnames:
    descriptor = WidgetProperty(pname)
    setattr(Widget, pname, descriptor)
    alt_name = pname.replace(' ', '_')
    if alt_name != pname:
        setattr(Widget, alt_name, descriptor)

for cname in constants.cmdnames:
    if hasattr(Widget, cname):
        continue
    meth = CommandMethod(cname)
    setattr(Widget, cname, meth)
    alt_name = cname.replace(' ', '_')
    if alt_name != cname:
        setattr(Widget, alt_name, meth)
