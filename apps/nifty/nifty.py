import PicoGUI, sys
fname = sys.argv[1]
app = PicoGUI.Application("This here is my text editor. Is it not nifty? Worship the text editor! (%s)" % fname)
t = app.addWidget('scrollbox').addWidget('Textbox','inside')
t.side = 'All'
f = open(fname, 'r')
t.text = f.read()
f.close()
panel = app.panelbar() or app
bt=panel.addWidget('Button', 'inside')
bt.text = 'Save'
def save (ev, button):
    f = open(fname, 'w')
    f.write(t.text)
app.link(save, bt, "activate")
app.run()
