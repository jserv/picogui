from Nifty.Workspace import Workspace
import persistence

class Field(persistence.Persistent):
    def __init__(self, name, widget='Field'):
        persistence.Persistent.__init__(self)
        self.name = name
        self.widget = widget

class SchemaViewer(Workspace):
    widget_type = 'box'
    
    def open(self, frame, page, schema):
        self.fields = {}
        last = self.addWidget('Label', 'inside')
        last.text = 'Poing Schema'
        for field in schema.fields:
            b = last.addWidget('Box')
            label = b.addWidget('Label', 'inside')
            label.text = field.name
            label.side = 'left'
            control = label.addWidget(field.widget)
            self.fields[field.name] = control
        Workspace.open(self, frame, page, schema)

    def update(self, schema):
        for field in schema.fields:
            self.fields[field.name].text = field.widget

class Viewer(Workspace):
    widget_type = 'box'
    
    def open(self, frame, page, buffer):
        self.fields = {}
        last = self.addWidget('Label', 'inside')
        last.text = 'Poing Data'
        for field in buffer.schema.fields:
            b = last.addWidget('Box')
            label = b.addWidget('Label', 'inside')
            label.text = field.name
            label.side = 'left'
            control = label.addWidget(field.widget)
            self.fields[field.name] = control
        Workspace.open(self, frame, page, buffer)

    def update(self, datum):
        for name, control in self.fields.items():
            control.text = getattr(datum, name)
