# Application class

import Widget, Server, events, time, infilter, template, struct

class EventHandled(Exception):
    """raise this from an event handler when you don't want other
    handlers to process this event"""
    pass

class StopApplication(Exception):
    """flag the event handler to stop"""
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
    
    def __init__(self, title='', server=None, handle=0):
        if not server:
            server = Server.Server()
        if self._type:
            Widget.Widget.__init__(self, server, server.register(title, self._type))
        else:
            Widget.Widget.__init__(self, server, handle)
        self.default_relationship = 'inside'
        self._widget_registry = {self.handle: self}
        self._event_registry = EventRegistry()
        self._event_stack = []
        self._infilter_registry = {}

    def panelbar(self):
        handle = self.__getattr__('panelbar')
        if handle:
            return Widget.Widget(self.server, handle, self)

    def createWidget(self, wtype):
        'convenience method to create an unparented widget'
        new_id = self.server.createWidget(wtype)
        new = Widget.Widget(self.server, new_id, self)
        self._notify_new_widget(new)
        return new

    def addInfilter(self, *args, **kw):
        filter = infilter.Infilter(self, *args, **kw)
        self._infilter_registry[filter.handle] = filter
        return filter

    def newTemplate(self, wt):
        t = template.Template(self, wt)
        return t

    def _notify_new_widget(self, new):
        self._widget_registry[new.handle] = new

    def link(self, handler, widget=None, evname=None):
        if type(widget) == type(''):
            evname = widget
            widget = None
        self._event_registry.add(handler, widget, evname)

    def delWidget(self, widget):
        widget.server.free(widget.handle)
        self._event_registry.remove(widget)
        if self._widget_registry.has_key(widget.handle):
            del self._widget_registry[widget.handle]

    def getWidget(self, handle):
        return self._widget_registry[handle]

    def send(self, widget, name, **attrs):
        self._event_stack.append(InternalEvent(name, widget, attrs))

    def poll_next_event(self):
        ev = self.server.wait()
        if ev.widget_id is None:
            if ev.name == 'infilter':
                ev = ev.trigger
                try:
                    ev.widget = self._infilter_registry[ev.sender]
                except KeyError:
                    ev.widget = self
            else:
                ev.widget = None
        else:
            try:
                ev.widget = self._widget_registry[ev.widget_id]
            except KeyError:
                ev.widget = Widget.Widget(self.server, ev.widget_id)
        self._event_stack.append(ev)

    def run(self):
        while 1:

            self.server.update()

            if self._event_registry.get(None, 'idle'):
                # if we have idle handlers, we want to see to it that they are called
                queued = self.server.checkevent()

                for i in range(queued):
                    self.poll_next_event()
                else: #nothing queued - send idle and sleep
                    self.send(self, 'idle')
                    time.sleep(0.1)
            else:
                # otherwise, just get one single event and dispatch it
                self.poll_next_event()

            try:
                self.dispatch_events()
            except StopApplication:
                break

    def dispatch_events(self):
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
                raise StopApplication

    def eventPoll(self):
        self.server.update()
        queued = self.server.checkevent()
        for i in range(queued):
            self.poll_next_event()
        self.dispatch_events()

    def shutdown(self):
        self.server.close_connection()

    def getVideoMode(self):
	mi = struct.unpack('!LHHHHHxx', self.server.getmode().data)
	return { 'flags': mi[0],
		 'xres': mi[1],
		 'yres': mi[2],
		 'lxres': mi[3],
		 'lyres': mi[4],
		 'bpp': mi[5] }


        
class ToolbarApp(Application):
    _type = 2

class InvisibleApp(Application):
    _type = 0  # do not register a window
    def addWidget(self, wtype, relationship=None):
        raise ValueError, "Invisible applications can't have widgets, silly!"

class TemplateApp(Application):
    # It's tricky to create an application from a widget template when
    # you need the server to load the template. This makes things easier.
    # It also gives you a handy way to import a list of widget names from
    # the template into properties of this object

    _type = 0  # do not register a window
    def __init__(self, wt, importList=[], server=None):
        if not server:
            server = Server.Server()
        self.template = server.mktemplate(wt)
        h = server.dup(self.template)
        Application.__init__(self,'',server,h)
        for name in importList:
            attrName = name.replace(' ','_')
            setattr(self,attrName,self.find(name))
    
