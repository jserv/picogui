#PicoGUI file manager. Filesystem abstracted interface.

import string
import os

#The generic file system interface defines an interface for
#the local disk. Override for other filesystems as appropriate
class filesystemInterface:
    def __init__(self, mime):
        self.path = '/'
        self.URI = "file://"
        self.mime = mime

    def listFiles(self):
        fileList = dict()
        for fileName in os.listdir(self.path):
            if os.path.isdir(os.path.join(self.path, fileName)):
                fileName = fileName+os.sep
                type = "Directory"
                fileList[fileName]=(fileName, type, None)
            else:
                try:
                    statData = os.stat(os.path.join(self.path, fileName))
                    size = statData.st_size
                    type = "File"
                    fileList[fileName] = (fileName, type, size)
                except:
                    print "Bad file"
        self.fileList = fileList
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
        newPath = string.join(pathList, os.sep)
        self.path = newPath

    def executeFile(self, filename, args):
        if os.fork() == 0:
            os.execl(os.path.join(self.path, filename), os.path.join(self.path, filename))

    def handleFile(self, filename):
        self.mime.callHandler(os.path.join(self.path, filename))

    def getMimeType(self, filename):
        return self.mime.getMimeType(os.path.join(self.path, filename))

    def getFileSize(self, filename):
        if self.fileList.has_key(filename):
            return self.fileList[filename][2]
    
    def isRootDir(self):
        if self.path == '/':
            return 1
        else:
            return 0

    def isExecutable(self, filename):
        fullPath = os.path.join(self.path, filename)
        if(self.fileList[filename][1] != "Directory"):
            if os.access(os.path.join(self.path, filename), os.X_OK) == 1:
                return 1
            else:
                return 0
        else:
            return 0

    def isDirectory(self, filename):
        if self.fileList[filename][1] == "Directory":
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
        print URI
        if self.filesystems.has_key(URI):
            return self.filesystems[URI]
        else:
            raise "FSI not found"

    def getDefaultFilesystem(self):
        return self.defaultFSI

