#!/usr/bin/env python

import PicoGUI, struct, os, sys, time
from PicoGUI import constants

class TerminalPage:
    def __init__(self, parent, relation, app, position):
        self.tabpage = parent.addWidget('tabpage', relation)
	self._terminal = self.tabpage.addWidget('terminal', 'inside')
	self._terminal.focus()
	self._termProcess = False
	self._app = app
	self._position = position
	if position < 12:
	    self.tabpage.hotkey = 'f%d' % (position+1)
	self._dynamictitle = False

	try:
            import pty, fcntl, termios
            self._fcntl = fcntl
            self._termios = termios
        
            (pid, fd) = pty.fork()
            if pid == 0:
                os.execlp("bash", "bash", "--login")
            self._ptyfd = fd
            self._ptypid = pid

            self._fcntl.fcntl(self._ptyfd, self._fcntl.F_SETFL, os.O_NDELAY)
	    self._termProcess = True
	    self._app.link(self.terminalHandler, self._terminal, 'data')
	    self._app.link(self.terminalResizeHandler, self._terminal, 'resize')
	    self._app.link(self.terminalTitleHandler, self._terminal, 'titlechange')
	    self._app.link(self.tabClicked, self.tabpage, 'activate')
	except:
	    self._terminal.write("The terminal isn't supported on this operating system.\n\r")

        app.server.poll(self.update, self._ptyfd)

    def terminalHandler(self, ev):
        os.write(self._ptyfd, ev.data)

    def terminalResizeHandler(self, ev):
        if ev.x > 0 and ev.y > 0:
	    self._fcntl.ioctl(self._ptyfd, self._termios.TIOCSWINSZ, struct.pack('4H', ev.y, ev.x, 0, 0))

    def terminalTitleHandler(self, ev):
        self.tabpage.text = ev.data
	self._dynamictitle = True
	if self._app.current == self._position:
	    self._app.text = self.tabpage.text

    def update(self, fd, event):
        self.terminalRead()
        # expire this terminal if pid is finished
	status = os.waitpid(self._ptypid, os.WNOHANG)
	if status[0] != 0:
            self.destroy()
	    os.close(self._ptyfd)

    def destroy(self):
        self._app.server.poll(None, self._ptyfd)
        self._app.delWidget(self.tabpage)
        self._app.destroy(self._position)

    def terminalRead(self):
        if self._termProcess:
	    try:
	        self._terminal.write(os.read(self._ptyfd, 4096))
	    except OSError:
	        pass

    def tabClicked(self, ev):
        self._terminal.focus()
	if self._dynamictitle:
	    self._app.text = self.tabpage.text
	else:
	    self._app.text = "epterm"
	self._app.current = self._position

    def setPosition(self, position):
        self._position = position


class Config:
    def __init__(self, app, parent, relation):
        self._parent = parent
        self._box = parent.addWidget('scrollbox', relation)
	self._palletetab = self._box.addWidget('tabpage', 'inside')
	self._palletetab.text = 'Pallete'
	self._bindtab = self._palletetab.addWidget('tabpage', 'after')
	self._bindtab.text = 'Keybindings'
	self._app = app
	self._app.link(self.activate, self._parent, 'activate')

    def activate(self, ev):
        self._app.text = "epterm config"


class App(PicoGUI.Application):
    def __init__(self):
        self._pages = []
	PicoGUI.Application.__init__(self, 'epterm')

	self._newtabhotkey = self.createWidget('button')
	self._newtabhotkey.hotkey = 'f12'
	self.link(self.addtab, self._newtabhotkey, 'activate')
	self._pages.append(TerminalPage(self, 'inside', self, 0))
	self._pages[-1].tabpage.text = 'terminal'

	self._current = 0

	self._config = self._pages[-1].tabpage.addWidget('tabpage', 'after')
	self._config.text = 'config'
	self._configstuff = Config(self, self._config, 'inside')

    def addtab(self,ev):
        self.appendtab()

    def destroy(self, position):
        del self._pages[position]
	i = position;
	if(len(self._pages) != 0):
	    while(i != len(self._pages)):
	        self._pages[i].setPosition(i)
	        i = i + 1
	    if position >= len(self._pages):
	        self._pages[position - 1].tabpage.on = 1
	    else:
	        self._pages[position].tabpage.on = 1

    def appendtab(self):
	self._pages.append(TerminalPage(self._config,'before', self, len(self._pages)))
	self._pages[-1].tabpage.text = 'tab!'
	self._pages[-1].tabpage.on = 1


if __name__ == '__main__':
    f = App()
    f.run()
