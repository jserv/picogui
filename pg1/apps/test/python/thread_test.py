#!/usr/bin/env python

import PicoGUI, threading, thread
PicoGUI.server.debug_threads = 1
app = PicoGUI.Application('Greetings')
print 'main thread is', thread.get_ident()

b = app.addWidget('Button')
b.side = 'Bottom'
b.text = 'poke'

def thread_run():
    app.run()

t = threading.Thread(None,
                     thread_run,
                     None)
t.start()

l = app.addWidget('Label')
l.side = 'All'
l.text = 'Hello, World!'
l.font = ':24:Bold'

print 'hibernating'
t.join()
print 'bye'
app.shutdown()
