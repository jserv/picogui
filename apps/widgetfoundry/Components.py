import PicoGUI


class PanelComponent:
    "Generic toggleable component in a panel"
    def _close(self, ev, widget):
        self.destroy()

    def __init__(self, main):
        self.widget = main.toolbar.widget.addWidget('panel')
        self.main = main
        self.widget.sizemode = 'percent'
        self.widget.size = 30
        main.app.link(self._close, self.widget, 'close')

    def destroy(self):
        self.main.app.delWidget(self.widget)
        self.toggleButton.on = 0


def toggleComponent(ev, widget):
    "Handler that can toggle on and off any UI component"
    if widget.on:
        component = widget.component(widget.main)
        component.toggleButton = widget
        widget.main.__dict__[widget.attr] = component
    else:
        widget.main.__dict__[widget.attr].destroy()
        del widget.main.__dict__[widget.attr]


class Toolbar:
    def __init__(self, main):
        "Create a toolbar with the buttons listed above"
        self.widget = main.app.addWidget('toolbar')
        self.main = main

    def add(self, buttons):
        buttons.reverse()
        for button in buttons:
            w = self.widget.addWidget('button','inside')
            w.text = button[0]
            self.main.app.link(button[1], w, 'activate')
            proplist = button[2]
            w.main = self.main
            for prop in proplist.keys():
                setattr(w,prop,proplist[prop])
