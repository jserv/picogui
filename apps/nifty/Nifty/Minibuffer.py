class Minibuffer(object):
    "A field that displays stuff and executes python statements"

    def __init__(self, frame):
        self._frame = frame
        self._field = frame.addWidget('field')
        self._field.side = 'bottom'
        self._history = []
        self._locals = {}
        self._globals = {'frame': frame}
        exec 'from Nifty import FileBuffer, ScratchBuffer' in self._globals

        frame.link(self._python_handler, self._field, 'activate')
        frame.link(self._key_handler, self._field, 'kbd keyup')

    def append_to_history(self):
        st = self._field.text
        if not st:
            return
        try:
            self._history.remove(st)
        except ValueError:
            pass
        self._history.append(st)

    def history_index(self):
        st = self._field.text
        if not st:
            # empty string - returning 0 will make the history behave as expected
            return 0
        try:
            i = self._history.index(st)
        except ValueError:
            self._history.append(st)
            i = -1
        return i

    def _python_handler(self, ev):
        self.append_to_history()
        self._field.text = ''
        self.bind(buffer = self._frame.current)
        try:
            exec self._history[-1] in self._globals, self._locals
        except SystemExit:
            raise
        except:
            import traceback
            traceback.print_exc()

    def _key_handler(self, ev):
        if ev.hasMod('ctrl'):
            print repr(ev)
            if ev.char == 'p':
                if not self._history:
                    # empty history
                    return
                pos = self.history_index() - 1
                self._field.text = self._history[pos]
            if ev.char == 'n':
                if not self._history:
                    # empty history
                    return
                pos = (self.history_index() + 1) % len(self._history)
                self._field.text = self._history[pos]

    def bind(__self, **kw):
        __self._globals.update(kw)
        # we use __self instead of self so that someone may bind the name 'self'
