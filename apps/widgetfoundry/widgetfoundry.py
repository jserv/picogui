#!/usr/bin/env python
import PicoGUI, Components, CommandLine, Toolbox


def saveXWT(ev, widget):
    print 'Ecky!'


class Main:
    def __init__(self):
        self.app = PicoGUI.Application('Widget Foundry')
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


class WorkArea:
    "This is where widget templates are constructed"
    def __init__(self, main):
        self.widget = main.toolbar.widget.addWidget('box')
        self.widget.side = 'all'
        self.main = main


if __name__ == '__main__':
    Main().app.run()
