from Buffer import Buffer
from Terminal import Terminal
import os, pty, fcntl, termios, struct

class Subprocess(Buffer):
    "Represents another process displaying in our workspace"

    widget = Terminal
    _argv = ("sh", "sh", "--login")

    def __init__(self, argv=None):
        if argv is not None:
            self._argv = argv
        (pid, fd) = pty.fork()
        if pid == 0:
            self._spawn()
        self._ptyfd = fd
        self._ptypid = pid
        self._new_text = None

        fcntl.fcntl(self._ptyfd, fcntl.F_SETFL, os.O_NDELAY)
        self._termProcess = True
        Buffer.__init__(self, name=self._argv[0], text='')

    def _spawn(self):
        # we're the child process, run something
        os.execlp(*self._argv)

    def notify_changed(self, ev):
        os.write(self._ptyfd, ev.data)
        self._write(ev.data, ev.widget)

    def _write(self, text, orig=None):
        self.text += text
        self._new_text = text
        self.update_all_observers_but(orig)
        self._new_text = None

    def update_observer(self, o):
        if self._new_text is not None:
            o.write(self._new_text)

    def save(self):
        print 'cannot save a Subprocess'

    def handle_resize(self, ev):
        if ev.x > 0 and ev.y > 0:
	    fcntl.ioctl(self._ptyfd, termios.TIOCSWINSZ, struct.pack('4H', ev.y, ev.x, 0, 0))

    def do_update(self, ev):
        if self._termProcess:
	    try:
	        text = os.read(self._ptyfd, 4096)
	    except OSError:
	        return
            self._write(text)
        