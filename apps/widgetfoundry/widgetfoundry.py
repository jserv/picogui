#!/usr/bin/env python
import PicoGUI
import Components, CommandLine, Toolbox, WTWidget


def saveXWT(ev, widget):
    print widget.main.selection.toXWT()


class Main:
    def __init__(self):
        self.app = PicoGUI.Application('Widget Foundry')
        self.selectNotify = {}
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
            ('Save XWT',       saveXWT, {} ),
            ])

    def select(self, widget):
        for i in self.selectNotify.keys():
            self.selectNotify[i](self.selection,widget)
        self.selection = widget


class WorkArea:
    "This is where widget templates are constructed"
    def __init__(self, main):
        self.widget = main.toolbar.widget.addWidget('box')
        self.widget.side = 'all'
        self.main = main
        
        w = WTWidget.Widget(main.app,self.widget.addWidget('label','inside'))
        main.select(w)

if __name__ == '__main__':
    Main().app.run()
