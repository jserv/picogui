# Helper class for widget templates

import Widget

class Template(object):
    def __init__(self, app, wt):
        self.app = app
        self.handle = app.server.mktemplate(wt)

    def instantiate(self, importList=[]):
        h = self.app.server.dup(self.handle)
        w = Widget.Widget(self.app.server, h, self.app)
        for name in importList:
            attrName = name.replace(' ','_')
            setattr(w,attrName,w.find(name))
        return w
    
