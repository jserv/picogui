import PicoGUI, sys
from code import InteractiveConsole
from Components import PanelComponent


class CommandLine(PanelComponent, InteractiveConsole):
    def __init__(self, main):
        PanelComponent.__init__(self,main)

        # Redirect stdout and stderr to our own write() function
        self.saved_stdout = sys.stdout
        self.saved_stderr = sys.stderr
        sys.stdout = self
        sys.stderr = self
        
        locals = {
            "__name__":   "__console__",
            "__doc__":    None,
            "main":       main,
            "PicoGUI":    PicoGUI,
            "sys":        sys,
            }
        InteractiveConsole.__init__(self,locals)

        try:
            sys.ps1
        except AttributeError:
            sys.ps1 = ">>> "
        try:
            sys.ps2
        except AttributeError:
            sys.ps2 = "... "

        self.widget.side = 'bottom'
        self.widget.text = 'Python Command Line'
        self.scroll = self.widget.addWidget('scrollbox','inside')
        self.textbox = self.scroll.addWidget('textbox','inside')
        self.textbox.readonly = 1
        self.textbox.insertmode = 'append'
        self.textbox.side = 'all'
        self.commandBox = self.widget.addWidget('box','inside')
        self.commandBox.side = 'bottom'
        self.prompt = self.commandBox.addWidget('label','inside')
        self.prompt.text = sys.ps1
        self.prompt.side = 'left'
        self.line = self.prompt.addWidget('field')
        self.line.side = 'all'
        main.app.link(self.enterLine, self.line, 'activate')

        self.write("Python %s on %s\n(Widget Foundry shell, See main.__dict__ for useful variables)\n" %
                   (sys.version, sys.platform))

    def destroy(self):
        sys.stdout = self.saved_stdout
        sys.stderr = self.saved_stderr

        self.main.app.delWidget(self.textbox)
        self.main.app.delWidget(self.scroll)
        self.main.app.delWidget(self.commandBox)
        self.main.app.delWidget(self.prompt)
        self.main.app.delWidget(self.line)
        PanelComponent.destroy(self)

    def write(self, data):
        self.textbox.text = data

    def enterLine(self, ev, widget):
        line = widget.text[:]
        self.write(self.prompt.text)
        self.write(line + '\n')
        if self.push(line):
            self.prompt.text = sys.ps2
        else:
            self.prompt.text = sys.ps1
        widget.text = ''
        
