#PicoGUI file manager. Mime thingy.

import os
import clamps_fsi
import string
import mimetypes

class mimeInterface:
    def __init__(self):
        self.handlerDict = dict()
        self.addHandler(('audio/mpeg', None), "/usr/bin/mpg123", ["<FILENAME>"])
        self.addHandler(('image/jpeg', None), "/home/carpman/projects/picogui/apps/imgview/imgview", ["<FILENAME>"])

    def getMimeType(self, filename):
        type = mimetypes.guess_type(filename)
        if type[0] == None:
            return "None/None"
        else:
            return type[0]

    def addHandler(self, mime, path, args):
        self.handlerDict[mime] = (path, args)

    def callHandler(self, filename):
        mimeType = mimetypes.guess_type(filename)
        print mimeType
        if self.handlerDict.has_key(mimeType):
            handlerData = self.handlerDict[mimeType]
            newArgs = list()
            newArgs.append(handlerData[0])
            for arg in handlerData[1]:
                if arg == "<FILENAME>":
                    newArgs.append(filename)
                else:
                    newArgs.append(arg)
            #should probably use fsi's version of this, later.
            if os.fork() == 0:
                os.execv(handlerData[0], newArgs)
            
