"""
 MiniSVN.py - A minimalist Subversion client capable of doing
              multithreaded atomic checkouts, using only the
              Python standard library and MiniDAV.
"""
# 
# PicoGUI Build System
# Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
# 
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#  
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#  
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
# 

from urlparse import *
import threading
from MiniDAV import DavObject
import os, re

class DownloaderThread(threading.Thread):
    def __init__(self, queue, queueSem):
        self.queue = queue
        self.queueSem = queueSem
        threading.Thread.__init__(self)

    def run(self):
        """Thread-safe queue management, to feed items into download()
           and enqueue children.
           """
        while True:
            self.queueSem.acquire()
            if len(self.queue) <= 0:
                self.queueSem.release()
                break
            queueItem = self.queue.pop()
            self.queueSem.release()
            newItems = self.download(queueItem)
            if newItems:
                self.queueSem.acquire()            
                for item in newItems:
                    self.queue.append(item)
                self.queueSem.release()

    def download(self, item):
        """Download an item from the queue, returning a list of
           more items to add to the queue.
           """
        (object, destination) = item

        if object.getType() == 'collection':
            # This object is a directory- recursively create it
            os.makedirs(destination)
            print " d %s" % destination

        elif object.getType() == 'file':
            # This object is a file- download it, creating the directory if necessary
            try:
                f = open(destination, "wb")
            except IOError:
                os.makedirs(os.path.join(os.path.split(destination)[:-1])[0])
                f = open(destination, "wb")                
            f.write(object.read())
            f.close()
            print "-> %s" % destination
        
        # Get our children ready to queue
        enqueue = []
        for child in object.getChildren():
            # Find the part of the child's URL that was (presumably)
            # appended to this object's URL and append it to the destination
            suffix = child.url[len(object.url):].replace("/","")
            enqueue.append((child, os.path.join(destination, suffix) ))
        return enqueue


class SVNRepository(DavObject):
    def __init__(self, url):
        # The caller may pass a URL of the form "/repository/!svn/bc/<revision>/path"
        # to check out a particular revision. If they haven't already specified a
        # special path like this, find the latest revision number and use that.
        # This part is what makes checkouts atomic.
        if url.find("!svn") < 0:
            # This part comes from guesstimation and looking at apache logs...
            # I couldn't find any docs on the WebDAV URLs used by Subversion, and
            # this was faster than digging through the source :)
            reposPath = DavObject(url).getProperties()['DAV::checked-in']
            reposPath = reposPath.replace("/!svn/ver/", "/!svn/bc/")
            parsed = urlparse(url)
            url = urlunparse((parsed[0], parsed[1], reposPath, '', '', ''))
        DavObject.__init__(self, url)

    def download(self, destination, numThreads=5):

        # If this repository consists of just one file, go ahead and join that to
        # the destination path.
        if len(self.getChildren()) == 0:
            suffix = re.search("/([^/]+)$", self.path).group(1)
            destination = os.path.join(destination, suffix)

        queue = [(self, destination)]
        queueSem = threading.Semaphore()
        threads = []
        for i in xrange(numThreads):
            thread = DownloaderThread(queue, queueSem)
            threads.append(thread)
            thread.start()
        for thread in threads:
            thread.join()


if __name__ == '__main__':
    import sys
    repo = SVNRepository(sys.argv[1])
    repo.download(sys.argv[2])
