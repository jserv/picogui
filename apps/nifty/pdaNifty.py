#!/usr/bin/env python
import sys, PicoGUI, thread, gc, os
from Nifty import Frame, FileBuffer, ScratchBuffer
import Nifty.Poing as Poing
import Nifty.QuikWriting as nqw

poing = Poing.DB()
frames = {}

main_out = sys.stdout
class _main_err(object):
    orig = sys.stderr
    tb = None
    def write(self, s):
        self.orig.write(s)
        if self.tb is None:
            app.server.mkcontext()
            self.dlg = app.createWidget('dialogbox')
            self.dlg.text = 'pdaNifty message'
            self.tb = self.dlg.addWidget('textbox', 'inside')
            self.tb.insertmode = 'append'
            btn = self.tb.addWidget('button')
            btn.text = 'ok'
            btn.side = 'all'
            app.link(self.close, btn, 'activate')
        self.tb.readonly = False
        self.tb.write(s)
        self.tb.readonly = True
        app.server.update()

    def close(self, ev):
        app.server.rmcontext()
main_err = _main_err()

class _threaded_stream_manager (object):
    def __init__(self, name, fallback):
        self.name = name
        self.fallback = fallback

    def write(self, s):
        frame = frames.get (thread.get_ident(), None)
        sink = getattr (frame, self.name, self.fallback)
        sink.write(s)

sys.stdout = _threaded_stream_manager ('minibuffer', main_out)
sys.stderr = _threaded_stream_manager ('stderr', main_err)

def run_frame(frame):
    id = thread.get_ident()
    frames[id] = frame
    frame.bind(poing = poing)
    frame.run()
    del frames[id]
    gc.collect()

def open_frame():
    frame = Frame("This here is my text editor. Is it not nifty? Worship the text editor!")
    thread.start_new_thread (run_frame, (frame,))
    return frame

def get_frame():
    if frames:
        frame = frames.values()[0]
    else:
        frame = open_frame()
    # doesn't work... how do we do something like that?
    #frame._app.focus()
    return frame

app = PicoGUI.InvisibleApp()
qwpad = nqw.QWPad(height=30, app=app)
qwpad.hide()
def vr3_silkscreen(t):
    if t.y < 243 or t.y > 300:
        return
    if t.x < 17:
        print 'menu'
    elif t.x < 41:
        print 'contacts'
    elif t.x < 65:
        print 'tasks'
    elif t.x < 90:
        print 'events'
    elif t.x < 111:
        get_frame()
    elif t.x < 139:
        print 'calc'
    else:
        #os.spawnlp(os.P_NOWAIT, 'pqw', 'pwq')
        qwpad.toggle_hide()
app.link(vr3_silkscreen,
         app.addInfilter(after=app.server.getresource('infilter pntr normalize'),
                         accept=('up',)))
print >>sys.stderr, 'ready'
app.run()
