import PicoGUI, sys

def open (fname, textbox):
    print 'opening "%s" in %s' % (fname, textbox)
    f = file(fname, 'r')
    textbox.text = f.read()
    f.close()
    textbox.filename = fname
    textbox.tabpage.text = fname

def new_textbox ():
    try:
        parent = tabs[-1]
        page = parent.addWidget('tabpage')
    except IndexError:
        page = box.addWidget('tabpage', 'inside')
        python.globals['tabbar'] = PicoGUI.Widget(app.server, page.tab_bar, app)
    tabs.append(page)
    t = page.addWidget('scrollbox', 'inside').addWidget('Textbox','inside')
    t.tabpage = page
    t.side = 'All'
    page.textbox = t
    return t

app = PicoGUI.Application("This here is my text editor. Is it not nifty? Worship the text editor!")
box = app.addWidget('box')
box.side = 'All'
tabs = []

bar = app.panelbar() or app.addWidget('toolbar')
bt = bar.addWidget('Button', 'inside')
bt.text = 'Save'

def save (ev, button):
    #f = open(fname, 'w')
    #f.write(t.text)
    print 'save which textbox?'

app.link(save, bt, "activate")

python = app.addWidget('field')
python.side = 'bottom'
python.locals = {}
python.globals = {'app': app, 'tabs': tabs, 'open': open, 'new_textbox': new_textbox}

def exec_python(ev, field):
    st = field.text
    field.text = ''
    try:
        exec st in field.globals, field.locals
    except SystemExit:
        app.send(app, 'stop')
    except:
        import traceback
        traceback.print_exc()

app.link(exec_python, python, 'activate')

if len(sys.argv) == 1:
    t = new_textbox()
    t.filename = ''
else:
    for name in sys.argv[1:]:
        open(name, new_textbox())

app.run()
