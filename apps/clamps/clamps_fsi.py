#PicoGUI file manager. Filesystem abstracted interface.

import string
import os

#The generic file system interface defines an interface for
#the local disk. Override for other filesystems as appropriate
class filesystemInterface:
    def __init__(self):
        self.path = '/'
        self.URI = "file://"

    def listFiles(self):
        fileList = list()
        for fileName in os.listdir(self.path):
            if os.path.isdir(os.path.join(self.path, fileName)):
                fileName = fileName+os.sep
                type = "Directory"
                fileList.append((fileName, type, None))
            else:
                statData = os.stat(os.path.join(self.path, fileName))
                size = statData.st_size
                type = "File"
                fileList.append((fileName, type, size))
        return fileList

    def getPath(self):
        return self.URI+self.path

    def setPath(self, path):
        string.replace(path, self.URI, '')
        print path 
        self.path = path

    def followDir(self, dirName):
        self.path = os.path.join(self.path, dirName)

    def upDir(self):
        pathList = string.split(self.path, os.sep)
        del pathList[len(pathList)-2]
        print pathList
        newPath = string.join(pathList, '/')
        print newPath
        self.path = newPath

    def isRootDir(self):
        if self.path == '/':
            return 1
        else:
            return 0
                          
class filesystemAbstractor:
    def __init__(self):
        self.filesystems = dict()
        self.defaultFSI = None
    
    def addFilesystem(self, URI, fsi, default=0):
        if not self.filesystems.has_key(URI):
            self.filesystems[URI] = fsi
            if default == 1:
                self.defaultFSI = fsi
        else:
            raise "FSI exists"
        
    def getFilesystem(self, URI):
        if self.filesystems.has_key(URI):
            return self.filesystems[URI]
        else:
            raise "FSI not found"

    def getDefaultFilesystem(self):
        return self.defaultFSI

fsi = filesystemInterface()
print fsi.listFiles()
