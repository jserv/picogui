# Widget class

default_relationship = 'after'

import Server, struct, types
_propnames = Server.constants['set'].keys()
_cmdnames = Server.constants['writecmd'][0].keys()

class Command_Proxy(object):
    def __init__(self, widget, name, ns=None, args=()):
        self.widget = widget
        self.name = name
        self.args = args
        if ns is None:
            ns = Server.constants['writecmd'][0][name][1]
        if type(ns) is types.DictType:
            self.ns = ns
        else:
            self.ns = None

    def __call__(self, *args):
        args = self.args + args
        self.widget.command(self.name, *args)

    def __getattr__(self, name):
        try:
            ns = self.ns[name][1]
        except KeyError:
            raise AttributeError(name)
        return Command_Proxy(self.widget, self.name, ns, self.args + (name,))

class Widget(object):
    def __init__(self, server, handle, parent=None):
        # we do this instead of self.server = server
        # to avoid calling __setattr__
        self.__dict__.update({
            'server': server,
            'handle': handle,
            'default_relationship': default_relationship,
            'parent': parent,
            })

    def __eq__(self, other):
        return (self.server is other.server) and (self.handle == other.handle)

    def _notify_new_widget(self, new):
        # do nothing, but this is overridden in Application to maintain a registry
        if self.parent and hasattr(self.parent, '_notify_new_widget'):
            self.parent._notify_new_widget(new)

    def addWidget(self, wtype, relationship=None):
        new_id = self.server.mkWidget(relationship or self.default_relationship, wtype, self.handle)
        new = Widget(self.server, new_id, self)
        self._notify_new_widget(new)
        return new

    def detach(self):
        self.attach(0)

    def attach(self, parent, relationship=None):
        self.server.attachwidget(parent.handle, self.handle, relationship or self.default_relationship)

    def find(self, name):
        # FIXME: This should find only widgets below this one in the hierarchy.
        #        This can be implemented here pending a protocol change in pgserver.
        return Widget(self.server,self.server.findwidget(name),self.parent)

    def stream(self, data):
        self.server.writedata(self.handle,data)

    def command(self, command, *parameters):
        self.server.writecmd(self.handle, command, *parameters)

    def __setattr__(self, name, value):
        pname = name.lower().replace('_', ' ')
        if pname in _propnames:
            self.server.set(self.handle, pname, value)
        else:
            self.__dict__[name] = value

    def __getattr__(self, name):
        pname = name.lower().replace('_', ' ')
        if pname in _propnames:
            result = self.server.get(self.handle, pname)
            ns = Server.constants['set'].get(pname)[1]
            return Server.unresolve_constant(result, ns, self.server)
        elif pname in _cmdnames:
            return Command_Proxy(self, pname)
        else:
            raise AttributeError(name)

