#!/usr/bin/env python
import PicoGUI
import Components, CommandLine, Toolbox, WTWidget


class Main:
    def __init__(self):
        self.app = PicoGUI.Application('Widget Foundry')
        self.selectNotify = {}
        self.changeNotify = {}
        self.selection = None

        # Give the window an initial size if it supports it
        try:
            self.app.width  = 800
            self.app.height = 600
        except PicoGUI.responses.ParameterError:
            pass

        self.toolbar = Components.Toolbar(self)
        self.workArea = WorkArea(self)

        self.toolbar.add([
            ('Toolbox',        Components.toggleComponent, {
                'extdevents' : 'toggle',
                'component'  : Toolbox.Toolbox,
                'attr'       : 'toolbox'
                }),
            ('Command Line',   Components.toggleComponent, {
                'extdevents' : 'toggle',
                'component'  : CommandLine.CommandLine,
                'attr'       : 'commandLine'
                }),
            ])

    def select(self, widget):
        for i in self.selectNotify.keys():
            self.selectNotify[i](self.selection,widget)
        self.selection = widget


class WorkArea:
    "This is where widget templates are constructed"
    def __init__(self, main):
        self.root = WTWidget.Widget(main.app,main.toolbar.widget.addWidget('box'))
        self.root.side = 'all'
        self.main = main
        self.root.changeNotify = self.changeNotify
        
        w1 = self.root.addWidget('label','inside')
        w2 = w1.addWidget('label')
        w3 = w2.addWidget('button')

        w1.text = "This..."
        w2.text = "... is a test!"
        w3.text = "Yay"
        w3.side = "top"
        
        main.select(w3)

    def changeNotify(self, widget, property):
        for i in self.main.changeNotify.keys():
            self.main.changeNotify[i](widget,property)
       

if __name__ == '__main__':
    Main().app.run()
