# Helper class for widget templates

import Widget

class Template(object):
    def __init__(self, app, wt):
        self.app = app
        self.server = app.server
        self.handle = app.server.mktemplate(wt)
        self.instances = []

    def instantiate(self, importList=[]):
        h = self.app.server.dup(self.handle)
        w = Widget.Widget(self.app.server, h, self.app)
        self.instances.append(w)
        for name in importList:
            attrName = name.replace(' ','_')
            setattr(w,attrName,w.find(name))
        w.importList = importList
        return w
    
    def destroy(self):
        self.server.free(self)
        for instance in self.instances:
            self.app.delWidget(instance)
            for i in instance.importList:
                self.app.delWidget(getattr(instance,i))
