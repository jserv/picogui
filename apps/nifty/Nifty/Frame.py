import PicoGUI, sys
from Minibuffer import Minibuffer
from DebugBuffer import DebugBuffer
from Textbox import Textbox
#import pax.backwards_compatibility

class Frame(object):
    "A window"

    history_limit = 23 # completely arbitrary

    def __init__(self, title):
        self._pages = []
        self._app = PicoGUI.Application(title)

        self._box = self.addWidget('Box')
        self._box.side = 'All'

        self.python_ns = {'frame': self}
        exec 'from Nifty import FileBuffer, ScratchBuffer' in self.python_ns

        bar = self._app.panelbar() or self.addWidget('toolbar')
        bt = bar.addWidget('Button', 'inside')
        bt.text = 'Save'

        self.link(self._save_button_handler, bt, 'activate')

        self.minibuffer = Minibuffer(self)
        sys.stdout = self.minibuffer

        #sys.stderr = DebugBuffer(self)

    def get_current(self):
        for page in self._pages:
            if page.on:
                return page.textbox

    def set_current(self, textbox):
        if not self._pages:
            return
        if type(textbox) in (int, long, float):
            page = self._pages[textbox]
        else:
            page = textbox.tabpage
        page.on = 1

    current = property(get_current, set_current, None, "currently selected textbox")

    def open(self, buffer):
        try:
            parent = self._pages[-1]
            page = parent.addWidget('tabpage')
        except IndexError:
            page = self._box.addWidget('tabpage', 'inside')
            self.bind(tabbar = PicoGUI.Widget(self._app.server, page.tab_bar,
                                              self._app, type='tabbar'))
        self._pages.append(page)
        t = page.addWidget('scrollbox', 'inside').addWidget('Textbox','inside', wrapper_class=Textbox)
        t.open(self, page, buffer)

    def close(self, box=None):
        if box is None:
            box = self.current
            is_current = 1
        else:
            is_current = box is self.current
        self._pages.remove(box.tabpage)
        box.buffer.del_observer(box)
        self._app.delWidget(box)
        self._app.delWidget(box.tabpage)
        if is_current:
            self.current = 0
        # for the sake of the garbage collector
        box.tabpage.textbox = None
        box.tabpage = None
        box.buffer = None

    def save(self, box=None):
        if box is None:
            box = self.current
        buffer = box.buffer
        buffer.text = box.text
        buffer.save()

    def _save_button_handler(self, ev):
        self.save()

    def bind(__self, **kw):
        __self.python_ns.update(kw)
        # we use __self instead of self so that someone may bind the name 'self'

    def link(self, *args):
        self._app.link(*args)

    def addWidget(self, *args):
        return self._app.addWidget(*args)

    def focus_textbox(self):
        if not self._pages:
            return
        self._app.server.focus(self.current)

    def run(self):
        return self._app.run()
