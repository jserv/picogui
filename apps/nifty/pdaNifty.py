#!/usr/bin/env python
import sys, PicoGUI, gc, os
from Nifty import Frame, FileBuffer, ScratchBuffer
import Nifty.Poing as Poing
import Nifty.QuikWriting as nqw

poing = Poing.DB()
frame = None

main_out = sys.stdout
class _main_err(object):
    orig = sys.stderr
    tb = None
    def write(self, s):
        self.orig.write(s)
        if self.tb is None:
            app.server.mkcontext()
            self.dlg = app.createWidget('dialogbox')
            # contexts phoock up cli_python's string cache, so don't use it
            self.dlg.text = app.server.mkstring('pdaNifty message')
            scroll = self.dlg.addWidget('scrollbox', 'inside')
            scroll.side = 'all'
            self.tb = scroll.addWidget('textbox', 'inside')
            self.tb.side = 'all'
            # this is a hack to make the dialog big enough
            self.tb.write('\n' * 10)
            btn = self.dlg.addWidget('button', 'inside')
            btn.text = app.server.mkstring('ok')
            btn.side = 'bottom'
            app.link(self.close, btn, 'activate')
            app.server.update()
            self.tb.write('')
            self.tb.insertmode = 'append'
        self.tb.readonly = False
        self.tb.write(s)
        self.tb.readonly = True
        app.server.update()

    def close(self, ev):
        app.server.rmcontext()
        self.tb = None
main_err = _main_err()
# uncomment when you need to debug
#main_err = sys.stderr

class _stream_manager (object):
    def __init__(self, name, fallback):
        self.name = name
        self.fallback = fallback

    def write(self, s):
        try:
            sink = getattr (frame, self.name, self.fallback)
            sink.write(s)
        except:
            import traceback
            print >> self.fallback, 'error while writing to %s!' % self.name
            traceback.print_exc(file=self.fallback)
            raise

sys.stdout = _stream_manager ('minibuffer', main_out)
sys.stderr = _stream_manager ('stderr', main_err)

def close_frame(ev):
    global frame
    print >> main_err, 'closing frame', frame
    _app = frame._app
    frame = None
    print >> main_err, 'forgotten'
    app.delWidget(_app) # does this leak? Do I have to del sub-widgets?
    print >> main_err, 'deleted'
    gc.collect()

def open_frame():
    global frame
    frame = Frame("This here is my text editor. Is it not nifty? Worship the text editor!", app=app)
    app.link(close_frame, frame, 'close')
    app.link(close_frame, frame, 'stop')
    frame.bind(poing = poing)

def get_frame():
    if frame is None:
        open_frame()
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
