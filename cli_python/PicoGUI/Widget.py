# Widget class

default_relationship = 'after'

import Server
_propnames = Server.constants['set'].keys()

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

    def writeTo(self, data):
        self.server.writeto(self.handle,data)

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
        else:
            raise AttributeError(name)
