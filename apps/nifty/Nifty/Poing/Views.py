from PicoGUI import Widget
from Nifty.Workspace import Workspace
import persistence

# TODO: support for removing fields from schemas

class Viewer(Workspace):
    widget_type = 'box'
    
    def open(self, frame, page, buffer):
        self.fields = {}
        self._last = self.addWidget('Label', 'inside')
        self._last.text = 'Poing %s' % buffer.title()
        Workspace.open(self, frame, page, buffer)
        if hasattr(buffer, 'addField'):
            b = self._last.addWidget('Box')
            b.side = 'bottom'
            self._add_name = b.addWidget('Field', 'inside')
            self._add_name.side = 'left'
            self._add_name.size = 10
            self._add_name.sizemode = 'percent'
            self._add_value = self._add_name.addWidget('Field')
            self._add_value.side = 'all'
            button = self._add_name.addWidget('Button')
            button.side = 'right'
            button.text = 'add'
            self.frame.link(self._do_add, button, 'activate')

    def _add_field(self, field):
        if self.fields.has_key(field.name):
            return
        self._last = self._last.addWidget('Box')
        label = self._last.addWidget('Label', 'inside')
        label.text = field.name
        label.side = 'left'
        control = label.addWidget(field.widget)
        self.fields[field.name] = control
        self.frame.link(self._changed, control, 'changed')

    def _do_add(self, ev):
        self.buffer.addField(self._add_name.text, self._add_value.text)
        self._add_name.text = ''
        self._add_value.text = ''
        self.update_from(self.buffer)

    def _changed(self, ev):
        self.frame.send(self, 'changed')

    def update_from(self, buffer):
        updated = []
        for field in buffer.Schema():
            self._add_field(field)
            self.fields[field.name].text = buffer.getField(field.name)
            updated.append(field.name)
        # now check for deletions

    def update(self, buffer):
        for field in buffer.Schema():
            buffer.setField(field.name, self.fields[field.name].text)
