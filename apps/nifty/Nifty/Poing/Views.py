from PicoGUI import Widget
from Nifty.Workspace import Workspace
import persistence

# TODO: support for adding and removing fields from schemas

class Viewer(Workspace):
    widget_type = 'box'
    
    def open(self, frame, page, buffer):
        self.fields = {}
        last = self.addWidget('Label', 'inside')
        last.text = 'Poing Editor'
        for field in buffer.Schema():
            last = last.addWidget('Box')
            label = last.addWidget('Label', 'inside')
            label.text = field.name
            label.side = 'left'
            control = label.addWidget(field.widget)
            self.fields[field.name] = control
            frame.link(self._changed, control, 'changed')
        Workspace.open(self, frame, page, buffer)
        #last = last.addWidget('Box')

    def _changed(self, ev):
        self.frame.send(self, 'changed')

    def update_from(self, buffer):
        for field in buffer.Schema():
            self.fields[field.name].text = buffer.getField(field.name)

    def update(self, buffer):
        for field in buffer.Schema():
            buffer.setField(field.name, self.fields[field.name].text)
