#PicoGUI file manager. Mime thingy.

import os
import clamps_fsi
import string
import mimetypes

class mimeInterface:
    def __init__(self):
        self.handlerDict = dict()
        self.addHandler(('audio/mpeg', None), "mpg123 <FILENAME>")
        self.addHandler(('image/jpeg', None), "imgview <FILENAME>")

    def getMimeType(self, filename):
        type = mimetypes.guess_type(filename)
        if type[0] == None:
            return "None/None"
        else:
            return type[0]

    def addHandler(self, mime, command):
        self.handlerDict[mime] = command

    def callHandler(self, filename):
        mimeType = mimetypes.guess_type(filename)
        print mimeType
        if self.handlerDict.has_key(mimeType):
            command = self.handlerDict[mimeType]
            command = command.replace("<FILENAME>",filename)
            #should probably use fsi's version of this, later.
            if os.fork() == 0:
                os.system(command)
            
