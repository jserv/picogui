#!/usr/bin/env python

# a version of Hello, World using the Server class

import PicoGUI

def test(address='localhost', display=0):
    server = PicoGUI.Server(address, display)
    top_id = server.register(server.getString('Greetings'))
    label_id = server.createWidget('Label')
    server.set(label_id, 'text', server.getString('Hello, World'))
    server.set(label_id, 'side', 'all')
    server.set(label_id, 'font', server.getFont(':24:Bold'))
    server.attachWidget(top_id, label_id, 'inside')
    server.update()
    return server

def event_loop(server):
    while 1:
        ev = server.wait()
        if ev.name == 'close':
            return

if __name__ == '__main__':
    from sys import argv
    if len(argv) > 2: display = argv[2]
    else: display = 0
    if len(argv) > 1: address = argv[1]
    else: address = 'localhost'
    c = test(address, display)
    event_loop(c)
