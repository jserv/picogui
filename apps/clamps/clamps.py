#PicoGUI file manager. Called clapms. Figure it out. :)

import PicoGUI
import os
import string
import clamps_fsi
import clamps_mime

class infoBox:
    def __init__(self, app):
        self.app = app
        self.infoLabels = dict()

        self.infoBoxW = self.app.addWidget('box')
        self.infoBoxW.side = 'bottom'
        self.infoBoxW.transparent = 1
        self.infoBoxW.margin = 0
        self.addInfo("filename", "File Name:", "---.---")
        self.addInfo("filesize", "File Size:", "0 kb")
        self.addInfo("mimetype", "Mime Type:", "None/None")

    def addInfo(self, infoName, infoLabel, emptyText):
        box = self.infoBoxW.addWidget("box", "inside")
        box.side = "right"
        infoLabelW = box.addWidget("label", "inside")
        infoLabelW.text = infoLabel
        infoLabelW.side = "left"
        self.infoLabels[infoName] = box.addWidget("label", "inside")
        self.infoLabels[infoName].text = emptyText
        self.infoLabels[infoName].side = "right"

    def setInfo(self, infoName, data):
        self.infoLabels[infoName].text = data
        self.app.server.update()

class fileButton:
    def __init__(self, filename, fsi, interface, upDir=0):
        self.filename = filename
        self.fsi = fsi
        self.interface = interface
        self.upDir = upDir
        self.selectedState = 0

        #draw widget
        self.fileBox = self.interface.dirview.addWidget("box", "inside")
        self.fileBox.side = "top"
        self.fileBox.transparent = 1
        self.fileBox.margin = 0
        if self.upDir == 0:
            self.fileItem = self.fileBox.addWidget("menuitem", "inside")
            self.fileItem.text = self.filename
            self.fileItem.align = "all"
            self.selected = self.fileBox.addWidget("checkbox", "inside")
            self.selected.side = "left"
            self.interface.app.link(self.fileSelect, self.selected, "activate")
            self.interface.app.link(self.handleFile, self.fileItem, "pntr up")
        else:
            self.fileItem = self.fileBox.addWidget("menuitem", "inside")
            self.fileItem.text = "Up to parent directory"
            self.fileItem.align = "all"
            self.interface.app.link(self.upDir, self.fileItem, "pntr up")

    def handleFile(self, ev, button):
        if self.interface.selectedFile != None and self.interface.selectedFile == self:
            if self.fsi.isExecutable(self.filename) == 1:
                self.fsi.executeFile(self.filename, list())
            elif self.fsi.isDirectory(self.filename) == 1:
                self.fsi.followDir(self.filename)
                self.interface.redraw()
            else:
                self.fsi.handleFile(self.filename)
        else:
            if self.interface.selectedFile != None:
                self.interface.selectedFile.unhilight()
            self.interface.selectedFile = self
            self.hilight()
            self.interface.infoBoxW.setInfo("filename", self.filename)
            self.interface.infoBoxW.setInfo("filesize", self.getFileSize())
            self.interface.infoBoxW.setInfo("mimetype", self.getMimeType())

    def fileSelect(self, ev, button):
        if self.selectedState == 1:
            self.interface.removeFile(self)
            self.selectedState = 0
        else:
            self.interface.addFile(self)
            self.selectedState = 1

    def upDir(self, ev, button):
        self.fsi.upDir()
        self.interface.redraw()

    def hilight(self):
        self.fileItem.hilight = 1

    def unhilight(self):
        self.fileItem.hilight = 0

    def getFileSize(self):
        return str(self.fsi.getFileSize(self.filename)) + " kb"

    def getMimeType(self):
        return str(self.fsi.getMimeType(self.filename))

    def __del__(self):
        self.interface.app.server.free(self.fileBox)
        self.interface.app.server.free(self.fileItem)
        if self.upDir == 0:
            self.interface.app.server.free(self.selected)
        self.interface.app.server.update()
        

class clampsInterface:
    def __init__(self, fsa):
        #The application
        self.app = PicoGUI.Application("Clamps")

        #Currently selected file
        self.selectedFile = None

        #Path variables
        self.diritems = list()

        #Checked file list
        self.fileList = list()

        #Filesystem abstractor
        self.fsa = fsa
        self.fsi = fsa.getDefaultFilesystem()

        #Dirview
        self.dirview = self.app.addWidget("scrollbox")

        #InfoBox
        self.infoBoxW = infoBox(self.app)

        #Toolbox
        self.toolbox = self.app.addWidget("toolbar")
        self.toolbox.side = "top"

        #Tools
        self.pathView = self.toolbox.addWidget("field", "inside")
        self.pathGo = self.toolbox.addWidget("button", "inside")
        self.pathGo.text = "Go"
        self.pathGo.side = "right"
        self.app.link(self.pathUpdate, self.pathGo, "activate")
        self.app.link(self.pathUpdate, self.pathView, "activate")

    def redraw(self):
        #Set the pathView location
        self.pathView.text = self.fsi.getPath()
        self.app.text = "Clamps - " + self.fsi.getPath()

        #Clean out the selected file
        self.selectedFile = None

        #Delete the old files
        for fileObject in self.diritems:
            fileObject.__del__()
        del self.diritems[0:len(self.diritems)-1]

        #Get updated list
        dirlist = self.fsi.listFiles()
        for fileInfo in dirlist:
            self.diritems.append(fileButton(fileInfo, self.fsi, self))
            
        #If this is not the root dir, make an updir button
        if not self.fsi.isRootDir():
            self.diritems.append(fileButton("", self.fsi, self, 1))

        #If we have a file selected, fill in it's info
        if self.selectedFile != None:
            self.infoBoxW.setInfo("filename", self.selectedFile.filename)
            self.infoBoxW.setInfo("filesize", self.selectedFile.getFileSize())
            self.infoBoxW.setInfo("mimetype", self.selectedFile.getMimeType())

        #update
        self.app.server.update()

    def pathUpdate(self, ev, button):
        newURL = self.app.server.getstring(self.pathView.text).data
        URLList = string.split(newURL, '://', 1)
        self.fsi = self.fsa.getFilesystem(URLList[0])
        self.fsi.setPath(URLList[1])
        self.redraw()

    def addFile(self, fileObj):
        self.fileList.append(fileObj)

    def removeFile(self, fileObj):
        fileCount = 0
        for checkedObj in self.fileList:
            if checkedObj == fileObj:
                del self.fileList[fileCount]
            fileCount = fileCount + 1
    
mime = clamps_mime.mimeInterface()
fsa = clamps_fsi.filesystemAbstractor()
file_fsi = clamps_fsi.filesystemInterface(mime)
fsa.addFilesystem("file", file_fsi, 1)
clamps = clampsInterface(fsa)
clamps.redraw()
clamps.app.run()
