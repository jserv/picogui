import PicoGUI
#import pax.backwards_compatibility

class Frame(object):
    "A window"

    def __init__(self, title):
        self._boxes = []
        self._app = PicoGUI.Application(title)

        self._box = self._app.addWidget('Box')
        self._box.side = 'All'

        bar = self._app.panelbar() or self._app.addWidget('toolbar')
        bt = bar.addWidget('Button', 'inside')
        bt.text = 'Save'

        self._app.link(self._save_button_handler, bt, 'activate')

        python = self._app.addWidget('field')
        python.side = 'bottom'
        python.locals = {}
        python.globals = {'frame': self}
        self._globals = python.globals
        exec 'from Nifty import FileBuffer, ScratchBuffer' in python.globals

        self._app.link(self._python_handler, python, 'activate')

    def get_current(self):
        for box in self._boxes:
            if box.on:
                return box

    def set_current(self, box):
        if type(box) in (int, long, float):
            box = self._boxes[box]
        box.on = 1

    current = property(get_current, set_current, None, "currently selected box")

    def open(self, buffer):
        try:
            parent = self._boxes[-1]
            page = parent.addWidget('tabpage')
        except IndexError:
            page = self._box.addWidget('tabpage', 'inside')
            self._globals['tabbar'] = PicoGUI.Widget(self._app.server, page.tab_bar, self._app, type='tabbar')
        self._boxes.append(page)
        t = page.addWidget('scrollbox', 'inside').addWidget('Textbox','inside')
        t.tabpage = page
        t.side = 'All'
        page.textbox = t
        t.buffer = buffer
        t.text = buffer.text
        t.tabpage.text = buffer.name

    def save(self):
        box = self.current.textbox
        buffer = box.buffer
        buffer.text = box.text
        buffer.save()
        print 'buffer %r saved' % buffer.name

    def _save_button_handler(self, ev, b):
        self.save()

    def _python_handler(self, ev, field):
        st = field.text
        field.text = ''
        self._globals['buffer'] = self.current
        try:
            exec st in field.globals, field.locals
        except SystemExit:
            self._app.send(self._app, 'stop')
        except:
            import traceback
            traceback.print_exc()

    def run(self):
        self._app.run()
