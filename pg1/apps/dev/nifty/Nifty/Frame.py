import PicoGUI, sys
from Minibuffer import Minibuffer
from DebugBuffer import DebugBuffer
import Nifty.config

file_detectors = []

class Frame(object):
    "A window"

    history_limit = 23 # completely arbitrary
    clipboard_limit = 23 # idem

    def __init__(self, title, initial_ns={}, app=None):
        self._pages = []
        self._app = PicoGUI.Application(title, parent=app)
        # for the benefit of someone who might wish to link events to us
        self.handle = self._app.handle

        self._box = self.addWidget('Box')
        self._box.side = 'All'

        self.python_ns = {'frame': self, '__name__': '__nifty__'}
        exec 'from Nifty import FileBuffer, ScratchBuffer, Subprocess, keybindings' in self.python_ns
        self.python_ns.update(initial_ns)

        Nifty.config.exec_config_file('init.py', self.python_ns)

        bar = self._app.panelbar() or self.addWidget('toolbar')
        bt = bar.addWidget('Button', 'inside')
        bt.text = 'Save'
        self.toolbar = bar

        self.link(self._save_button_handler, bt, 'activate')

        self.minibuffer = Minibuffer(self)
        self.stderr = DebugBuffer(self)

        self._clipboard = []

    def get_current(self):
        for page in self._pages:
            if page.on:
                return page.workspace

    def set_current(self, workspace):
        if not self._pages:
            return
        if type(workspace) in (int, long, float):
            page = self._pages[workspace]
        else:
            page = workspace.tabpage
        page.on = 1

    current = property(get_current, set_current, None, "currently selected workspace")

    def open(self, buffer):
        try:
            parent = self._pages[-1]
            page = parent.addWidget('tabpage')
        except IndexError:
            page = self._box.addWidget('tabpage', 'inside')
            self.bind(tabbar = PicoGUI.Widget(self._app.server, page.tab_bar,
                                              self._app, type='tabbar'))
        self._pages.append(page)
        ws = page.addWidget('scrollbox', 'inside').addWidget(buffer.widget, 'inside')
        ws.open(self, page, buffer)
        return ws

    def open_file(self, path):
        for detector in file_detectors:
            buffer = detector(path)
            if buffer is not None:
                self.open(buffer)
                print buffer.__class__.__name__
                return
        print 'No detectors were successful'

    def close(self, workspace=None):
        if workspace is None:
            workspace = self.current
            is_current = 1
        else:
            is_current = workspace is self.current
        self._pages.remove(workspace.tabpage)
        workspace.buffer.del_observer(workspace)
        self._app.delWidget(workspace)
        self._app.delWidget(workspace.tabpage)
        if is_current:
            self.current = 0
        # for the sake of the garbage collector
        workspace.tabpage.workspace = None
        workspace.tabpage = None
        workspace.buffer = None

    def save(self, workspace=None):
        if workspace is None:
            workspace = self.current
        buffer = workspace.buffer
        buffer.text = workspace.text
        buffer.save()

    def _save_button_handler(self, ev):
        self.save()

    def bind(__self, **kw):
        __self.python_ns.update(kw)
        # we use __self instead of self so that someone may bind the name 'self'

    def link(self, *args):
        self._app.link(*args)

    def send(self, *args):
        self._app.send(*args)

    def poll(self, handler, fd, mask='r'):
        self._app.server.poll(handler,fd,mask)

    def addWidget(self, *args):
        return self._app.addWidget(*args)

    def delWidget(self, *args):
        return self._app.delWidget(*args)

    def workspaces(self):
        return [page.workspace for page in self._pages]

    def __ask_confirm_cb(self, ev):
        self.__ask_confirm_answer = ev.widget.text
        raise SystemExit

    def ask_confirm(self, question, options):
        widgets = []
        widgets.append(self._app.createWidget('dialogbox'))
        widgets[0].text = question
        widgets[0].default_relationship = 'inside'
        for answer in options:
            widgets.append(widgets[-1].addWidget('button'))
            widgets[-1].text = answer
            self._app.link(self.__ask_confirm_cb, widgets[-1], 'activate')
        self._app.run()
        widgets.reverse()
        for w in widgets:
            # we delete them all because we don't want leftover
            # event bindings in the event registry
            self._app.delWidget(w)
        answer = self.__ask_confirm_answer
        del self.__ask_confirm_answer
        return answer

    def copy(self, data):
        self._clipboard.append(data)
        if len(self._clipboard) > self.clipboard_limit:
            self._clipboard = self._clipboard[-self.clipboard_limit:]

    def paste(self, offset=1):
        if not self._clipboard:
            print 'nothing to paste'
            return
        offset = offset % len(self._clipboard)
        return self._clipboard[-offset]

    def run(self):
        _stop = False
        while not _stop:
            r = self._app.run()
            _stop = True
            for page in self._pages:
                if _stop and hasattr(page.workspace, 'confirm_close'):
                    _stop = _stop and page.workspace.confirm_close()
        self._app.shutdown()
        return r
