# Application class

import Widget, Server, events, time

class EventHandled(Exception):
    """raise this from an event handler when you don't want other
    handlers to process this event"""
    pass

class InternalEvent(events.Event):
    def __init__(self, name, widget, attrs):
        self.__dict__.update(attrs)
        self.name = name
        self.widget = widget
        self.widget_id = widget.handle

class EventRegistry(object):
    def __init__(self):
        self.map = {}

    def get(self, widget, evname):
        if widget is None:
            handle = None
        else:
            handle = widget.handle
        return self.map.setdefault(handle, {}).setdefault(evname, [])

    def add(self, handler, widget=None, evname=None):
        self.get(widget, evname).append((handler, widget))

    def remove(self, widget):
        if self.map.has_key(widget):
            del self.map[widget]

    def dispatch(self, ev):
        widget = ev.widget
        name = ev.name
        l = self.map.get(None, {}).get(None, [])
        l += self.map.get(None, {}).get(name, [])
        if widget:
            l += self.map.get(widget.handle, {}).get(name, [])
            l += self.map.get(widget.handle, {}).get(None, [])
        for handler, wo in l:
            try:
                handler(ev, wo or widget)
            except EventHandled:
                return

class Application(Widget.Widget):
    _type = 1 # apptype parameter for the register request
    
    def __init__(self, title, server=None):
        if not server:
            server = Server.Server()
        Widget.Widget.__init__(self, server, server.register(title, self._type))
        self.default_relationship = 'inside'
        self._widget_registry = {self.handle: self}
        self._event_registry = EventRegistry()
        self._event_stack = []

    def _notify_new_widget(self, new):
        self._widget_registry[new.handle] = new

    def link(self, handler, widget=None, evname=None):
        if type(widget) == type(''):
            evname = widget
            widget = None
        self._event_registry.add(handler, widget, evname)

    def delWidget(self, widget):
        self._event_registry.remove(widget)
        if self._widget_registry.has_key(widget.handle):
            del self._widget_registry[widget.handle]

    def send(self, widget, name, **attrs):
        self._event_stack.append(InternalEvent(name, widget, attrs))

    def run(self):
        self.server.update()
        while 1:

            queued = self.server.checkevent()

            for i in range(queued):
                ev = self.server.wait()
                if ev.widget_id in self._widget_registry.keys():
                    ev.widget = self._widget_registry[ev.widget_id]
                elif ev.widget_id is None:
                    ev.widget = None
                else:
                    ev.widget = Widget.Widget(self.server, ev.widget_id)
                self._event_stack.append(ev)
            else: #nothing queued - send idle and sleep
                self.send(self, 'idle')
                time.sleep(0.1)
            # XXX DANGER for thread-safety
            # (could lose events - when we decide to go thread-safe this operation
            # needs to be wrapped in a semaphore)
            events = self._event_stack
            self._event_stack = []
            # XXX /DANGER
            for ev in events:
                try:
                    self._event_registry.dispatch(ev)
                except EventHandled:
                    continue
                if ev.widget is self and ev.name in ('close', 'stop'):
                    return

    def shutdown(self):
        self.server.close_connection()

class ToolbarApp(Application):
    _type = 2
