#!/usr/bin/env python

import PicoGUI, struct, os, sys
from PicoGUI import constants

class TerminalPage:
    def __init__(self, parent, relation, app, position):
        self.tabpage = parent.addWidget('tabpage', relation)
	self._terminal = self.tabpage.addWidget('terminal', 'inside')
	self._terminal.focus()
	self._termProcess = False
	self._app = app
	self._position = position
	if position == 0:
	    self.tabpage.hotkey = 'f1'
	if position == 1:
	    self.tabpage.hotkey = 'f2'

	try:
            import pty, fcntl, termios
            self._fcntl = fcntl
            self._termios = termios
        
            (pid, fd) = pty.fork()
            if pid == 0:
                os.execlp("sh", "sh", "--login")
            self._ptyfd = fd
            self._ptypid = pid

            self._fcntl.fcntl(self._ptyfd, self._fcntl.F_SETFL, os.O_NDELAY)
	    self._termProcess = True
	    self._app.link(self.terminalHandler, self._terminal, 'data')
	    self._app.link(self.terminalResizeHandler, self._terminal, 'resize')
	    self._app.link(self.tabClicked, self.tabpage, 'activate')
	except:
	    self._terminal.write("The terminal isn't supported on this operating system.\n\r")

    def terminalHandler(self, ev):
        os.write(self._ptyfd, ev.data)

    def terminalResizeHandler(self, ev):
        if ev.x > 0 and ev.y > 0:
	    self._fcntl.ioctl(self._ptyfd, self._termios.TIOCSWINSZ, struct.pack('4H', ev.y, ev.x, 0, 0))

    def update(self):
        self.terminalRead()
        # expire this terminal if pid is finished
	status = os.waitpid(self._ptypid, os.WNOHANG)
	if status[0] != 0:
	    self._app.delWidget(self.tabpage)
	    self._app.destroy(self._position)
	    os.close(self._ptyfd)

    def terminalRead(self):
        if self._termProcess:
	    try:
	        self._terminal.write(os.read(self._ptyfd, 4096))
	    except OSError:
	        pass

    def tabClicked(self, ev):
        self._terminal.focus()

    def setPosition(self, position):
        self._position = position

class App(PicoGUI.Application):
    def __init__(self):
        self._pages = []
	PicoGUI.Application.__init__(self, 'epterm')
	self._toolbar = self.addWidget('toolbar')

	self._newtabhotkey = self.createWidget('button')
	self._newtabhotkey.hotkey = 'f12'
	self.link(self.addtab, self._newtabhotkey, 'activate')
	self._pages.append(TerminalPage(self._toolbar, 'after', self, 0))
	self._pages[-1].tabpage.text = 'tab!'

    def addtab(self,ev):
        self.appendtab()

    def destroy(self, position):
        self._pages = self._pages[0:position] + self._pages[position + 1:]
	i = position;
	if(len(self._pages) != 0):
	    while(i != len(self._pages)):
	        self._pages[i].setPosition(i)
	        i = i + 1
	    self._pages[position].tabpage.on = 1

    def appendtab(self):
	self._pages.append(TerminalPage(self._pages[-1].tabpage,'after', self, len(self._pages)))
	self._pages[-1].tabpage.text = 'tab!'
	self._pages[-1].tabpage.on = 1

    def update(self):
        import time
        self.eventPoll()
	i = 0
	while(i != len(self._pages)):
	    self._pages[i].update()
	    if(len(self._pages) == 0):
	        sys.exit(0)
	    i = i + 1
	time.sleep(0.001)

f = App()
while(1):
    f.update()
