import PicoGUI
from Minibuffer import Minibuffer
#import pax.backwards_compatibility

class Frame(object):
    "A window"

    history_limit = 23 # completely arbitrary

    def __init__(self, title):
        self._pages = []
        self._app = PicoGUI.Application(title)

        self._box = self.addWidget('Box')
        self._box.side = 'All'

        bar = self._app.panelbar() or self.addWidget('toolbar')
        bt = bar.addWidget('Button', 'inside')
        bt.text = 'Save'

        self.link(self._save_button_handler, bt, 'activate')

        self.minibuffer = Minibuffer(self)

    def get_current(self):
        for page in self._pages:
            if page.on:
                return page.textbox

    def set_current(self, textbox):
        if type(textbox) in (int, long, float):
            page = self._pages[page]
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
            self.minibuffer.bind(tabbar = PicoGUI.Widget(self._app.server, page.tab_bar,
                                                         self._app, type='tabbar'))
        self._pages.append(page)
        t = page.addWidget('scrollbox', 'inside').addWidget('Textbox','inside')
        t.tabpage = page
        t.side = 'All'
        page.textbox = t
        t.buffer = buffer
        t.text = buffer.text
        t.tabpage.text = buffer.name

    def save(self):
        box = self.current
        buffer = box.buffer
        buffer.text = box.text
        buffer.save()
        print 'buffer %r saved' % buffer.name

    def _save_button_handler(self, ev):
        self.save()

    def link(self, *args):
        self._app.link(*args)

    def addWidget(self, *args):
        return self._app.addWidget(*args)

    def run(self):
        return self._app.run()
