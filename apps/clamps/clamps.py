#PicoGUI file manager. Called clapms. Figure it out. :)

import PicoGUI
import os
import string
import clamps_fsi

class contentManager:
    def handleFile(self, extension, fullPath):
        print "Got request to handle: " + extension

class clampsInterface:
    def __init__(self, fsa, contentHandler):
        #The application
        self.app = PicoGUI.Application("Clamps")

        #Currently selected file
        self.selectedFile = None

        #Path variables
        self.diritems = list()

        #Content handler
        self.contentHandler = contentHandler

        #Filesystem abstractor
        self.fsa = fsa
        self.fsi = fsa.getDefaultFilesystem()

        #InfoBox
        self.infoBox = self.app.addWidget("box")
        self.infoBox.sizemode = "percent"
        self.infoBox.size = "30"

        #Toolbox
        self.toolbox = self.app.addWidget("box")
        self.toolbox.side = "top"
        self.toolbox.sizemode = "percent"
        self.toolbox.size = "20"

        #Tools
        self.pathView = self.toolbox.addWidget("field", "inside")
        self.pathGo = self.toolbox.addWidget("button", "inside")
        self.pathGo.text = "Go"
        self.pathGo.side = "right"
        self.app.link(self.pathUpdate, self.pathGo, "activate")
        #self.app.link(self.pathView, self.pathGo, "activate")

        #Dirview
        metaview = self.app.addWidget("box")
        metaview.sizemode = "percent"
        metaview.size = 70
        metaview.side = "bottom"
        self.dirview = metaview.addWidget("box", "inside")
        self.dirview.side = "top"
        scroll = metaview.addWidget("scroll", "inside").bind = self.dirview

    def redraw(self):
        #Set the pathView location
        self.pathView.text = self.fsi.getPath()
        self.app.text = "Clamps - " + self.fsi.getPath()

        #Delete the old files
        for item in self.diritems:
            self.app.server.free(item)
        self.app.server.update()

        #Get updated list
        dirlist = self.fsi.listFiles()
        for fileInfo in dirlist:
            widget = self.dirview.addWidget("flatbutton", "inside")
            widget.text = fileInfo
            widget.side = "top"
            widget.align = "left"
            self.diritems.append(widget)
            self.app.link(self.handleFile, widget, "activate")
            
        #If this is not the root dir, make a updir button
        if not self.fsi.isRootDir():
            widget = self.dirview.addWidget("flatbutton", "inside")
            widget.text = "Up to parent directory"
            widget.side = "top"
            widget.align = "left"
            self.diritems.append(widget)
            self.app.link(self.upDir, widget, "activate")
        self.app.server.update()

    def upDir(self, ev, button):
        self.fsi.upDir()
        self.redraw()

    def pathUpdate(self, ev, button):
        newURL = self.app.server.getstring(self.pathView.text)[:-1]
        URLList = string.split(newURL, '://', 1)
        self.fsi = self.fsa.getFilesystem(URLList[0])
        self.fsi.setPath(URLList[1])
        self.redraw()

    def handleFile(self, ev, button):
        if self.selectedFile != None and self.selectedFile == button:
            filename = self.app.server.getstring(button.text)[:-1]
            if self.fsi.isExecutable(filename) == 1:
                self.fsi.executeFile(filename, list())
            else:
                self.fsi.followDir(filename)
                self.redraw()
        else:
            self.selectedFile = button
            button.on = 1

    def changedir(self, ev, button):
        self.path = self.path+self.app.server.getstring(button.text)[:-1]
        self.redraw()

content = contentManager()
fsa = clamps_fsi.filesystemAbstractor()
file_fsi = clamps_fsi.filesystemInterface()
fsa.addFilesystem("file", file_fsi, 1)
clamps = clampsInterface(fsa, content)
clamps.redraw()
clamps.app.run()
