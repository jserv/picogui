class Minibuffer(object):
    "A field that displays stuff and executes python statements"

    def __init__(self, frame):
        self._frame = frame
        self._field = frame.addWidget('field')
        self._field.side = 'bottom'
        self._saved_text = None
        self._history = []
        self.python_ns = {}

        frame.link(self._python_handler, self._field, 'activate')
        frame.link(self._key_handler, self._field, 'kbd keyup')
        frame.link(self._focus_handler, self._field, 'focus')

    def append_to_history(self):
        st = self._field.text
        if not st:
            return
        try:
            self._history.remove(st)
        except ValueError:
            pass
        self._history.append(st)
        if len(self._history) > self._frame.history_limit:
            del self._history[0]

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
        try:
            self._frame.current.focus()
        except AttributeError:
            # no buffers open
            self._frame.toolbar.focus()
        self._saved_text = None
        try:
            workspace = self._frame.current
            self.bind(buffer = workspace.buffer, workspace = workspace)
        except AttributeError:
            # no buffers open
            self.bind(buffer = None, workspace = None)
        try:
            c = compile(self._history[-1], '<string>', 'single')
            exec c in self._frame.python_ns, self.python_ns
        except SystemExit:
            raise
        except:
            import traceback
            traceback.print_exc()

    def _key_handler(self, ev):
        if ev.hasMod('ctrl'):
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

    def _focus_handler(self, ev):
        if self._saved_text is not None:
            self._field.text = self._saved_text
        self._saved_text = None

    def write(self, text):
        text = text.strip()
        if not text:
            return
        if self._saved_text is None:
            self._saved_text = self._field.text
        self._field.text = text
        try:
            self._frame.current.focus()
        except AttributeError:
            # no buffers open
            self._frame.toolbar.focus()

    def clear(self):
        self._field.text = ''

    def bind(__self, **kw):
        __self.python_ns.update(kw)
        # we use __self instead of self so that someone may bind the name 'self'

    def focus(self):
        self._field.focus()
