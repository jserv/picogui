# Application class

import Widget, Server, events, time, infilter, template, struct, sys
try:
    import thread
except:
    thread = None

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
                try:
                    handler(ev)
                except TypeError:
                    e = sys.exc_info()
                    try:
                        handler(ev, wo or widget)
                    except TypeError:
                        # raise the original exception, not the new one
                        raise e[0], e[1], e[2]
                    import warnings
                    warnings.warn('handler %r expects 2 arguments' % handler,
                                  DeprecationWarning,
                                  stacklevel = 4)
            except EventHandled:
                return

class Application(Widget.Widget):
    _type = 1 # apptype parameter for the register request

    # No idle events by default- if you want idle events, set
    # this to a delay in seconds (floating point) between event polls.
    # If this is None, a blocking event wait is performed rather than a poll.
    idle_delay = None
    
    def __init__(self, title='', server=None, handle=0, parent=None):
        if not server:
            if parent:
                server = parent.server
            else:
                server = Server.Server()
        if handle or not self._type:
            Widget.Widget.__init__(self, server, handle, parent)
        else:
            Widget.Widget.__init__(self, server, server.register(title, self._type), parent)
        self.default_relationship = 'inside'
        if parent is None:
            self._widget_registry = {self.handle: self}
            self._event_registry = EventRegistry()
            self._event_stack = []
            self._infilter_registry = {}
            if thread is not None:
                self._run_lock = thread.allocate_lock()
                self._dispatch_lock = thread.allocate_lock()
            self.app = self
        else:
            parent._notify_new_widget(self)
            self._widget_registry = parent._widget_registry
            self._event_registry = parent._event_registry
            self._event_stack = parent._event_stack
            self._infilter_registry = parent._infilter_registry
            if thread is not None:
                self._run_lock = parent._run_lock
                self._dispatch_lock = parent._dispatch_lock
            self.app = parent.app

    def panelbar(self):
        handle = self.server.get(self.handle, 'panelbar')
        if handle:
            try:
                return self.getWidget(handle)
            except KeyError:
                return Widget.Widget(self.server, handle, self)

    def createWidget(self, wtype, wrapper_class=Widget.Widget):
        'convenience method to create an unparented widget'
        name, factory = self._get_widget_factory(wtype, wrapper_class)
        new_id = self.server.createWidget(name)
        new = factory(self.server, new_id, self, type=name)
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
        if type(widget) in (type(0), type(0L)):
            try:
                widget = self.getWidget(widget)
            except KeyError:
                widget = Widget.Widget(self.server, widget, self)
        self._event_registry.add(handler, widget, evname)

    def delWidget(self, widget):
        if hasattr(widget, 'handle'):
            handle = widget.handle
            server = widget.server
            self._event_registry.remove(widget)
        else:
            handle = widget
            server = self.server
        server.free(handle)
        if self._widget_registry.has_key(handle):
            del self._widget_registry[handle]

    def getWidget(self, handle):
        return self._widget_registry[handle]

    def send(self, widget, name, **attrs):
        self._event_stack.append(InternalEvent(name, widget, attrs))

    def poll_next_event(self, timeout=None):
        ev = self.server.wait(timeout)
        if ev is None:
            return
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
        if thread is not None:
            self._run_lock.acquire()
        while 1:
            # Update the UI before waiting on the user
            self.server.update()
    
            if self.idle_delay != None:
                # Polling event loop
                queued = self.server.checkevent()
                if queued:
                    for i in range(queued):
                        self.poll_next_event()
                else:
                    queued = None
                    while not queued:
                        idle = InternalEvent('idle', self, {})
                        self._event_registry.dispatch(idle)
                        time.sleep(self.idle_delay)
                        queued = self.server.checkevent()
                    self.poll_next_event()

            else:
                # Blocking event loop
                self.poll_next_event()

            try:
                self.dispatch_events()
            except StopApplication, ret:
                if thread is not None:
                    self._run_lock.release()
                if ret.args:
                    return ret.args[0]
                return
            except:
                if thread is not None:
                    self._run_lock.release()
                raise

    def dispatch_events(self):
        if thread is not None:
            self._dispatch_lock.acquire()
        events = self._event_stack
        self._event_stack = []
        if thread is not None:
            self._dispatch_lock.release()
        for ev in events:
            try:
                self._event_registry.dispatch(ev)
            except SystemExit, e:
                if e.args:
                    raise StopApplication, e.args[0]
                else:
                    raise StopApplication
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
    
