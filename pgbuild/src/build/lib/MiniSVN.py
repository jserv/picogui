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
import os, re, time

class DownloaderPool:
    def __init__(self, queue, numThreads):
        """Starts multiple DownloaderThreads and stores data shared between them."""
        self.queue = queue
        self.queueSem = threading.Semaphore()
        self.numSleeping = 0
        self.threads = []
        self.running = 1

        # Create and start all threads
        for i in xrange(numThreads):
            thread = DownloaderThread(self)
            self.threads.append(thread)
            thread.start()

        # Wait for all threads to quit
        for thread in self.threads:
            thread.join()

class DownloaderThread(threading.Thread):
    def __init__(self, pool):
        self.pool = pool
        threading.Thread.__init__(self)

    def run(self):
        """Thread-safe queue management, to feed items into download()
           and enqueue children.
           """
        while self.pool.running:
            self.pool.queueSem.acquire()            
            try:
                queueItem = self.pool.queue.pop()
            except IndexError:
                # Nothing in the queue. If all the threads are in this state, we're done
                self.pool.queueSem.release()
                if self.pool.numSleeping == len(self.pool.threads)-1:
                    break
                self.pool.numSleeping += 1
                time.sleep(0.1)
                self.pool.numSleeping -= 1
                continue
            self.pool.queueSem.release()
            newItems = self.download(queueItem)
            if newItems:
                self.pool.queueSem.acquire()            
                for item in newItems:
                    self.pool.queue.append(item)
                self.pool.queueSem.release()
        self.pool.running = 0

    def download(self, item):
        """Download an item from the queue, returning a list of
           more items to add to the queue.
           """
        (object, destination) = item

        if object.getType() == 'collection':
            # This object is a directory- recursively create it
            try:
                os.makedirs(destination)
            except OSError:
                # We don't care if it already exists
                pass
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

        DownloaderPool([(self, destination)], numThreads)


if __name__ == '__main__':
    import sys
    repo = SVNRepository(sys.argv[1])
    repo.download(sys.argv[2])
