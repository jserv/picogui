""" PGBuild.Site

Interfaces for managing lists of download sites, including testing
site speed and picking mirrors.
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

import PGBuild.Errors
import urlparse, urllib2, time, random


def findSite(config, name):
    """Look up a site name, returning a list of <a> tags.
       This can be passed a <site> tag or a simple name string.
       """
    if type(name) != str:
        name = name.attributes['name'].value
    return config.xpath("sites/site[@name='%s']/a" % name)


def urljoin(a, b):
    """Our own version of urlparse.urljoin(). Forces the first argument
       to be interpreted as a directory name, to give semantics closer
       to os.path.join()
       """
    if a[-1] != '/':
        a = a + '/'
    return urlparse.urljoin(a,b)


class Location:
    """Encapsulate one download location. Stores the aboslute URI for
       that location and reference to the <a> tag for its host.
       Includes methods to perform speed testing.
       """
    def __init__(self, config, absoluteURI, host):
        self.config = config
        self.absoluteURI = absoluteURI
        self.host = host

    def testSpeed(self):
	"""Try to retrieve the first few kilobytes of the URL, measuring
           the speed. If the site is down, set the speed to zero.
           """
        try:
            bytes = 0
            startTime = time.time()
            url = urllib2.urlopen(self.absoluteURI)
            bytes += len(url.read(64000))
            url.close()
            endTime = time.time()
            speed = bytes / (endTime - startTime)
        except IOError:
            speed = 0
        PGBuild.XMLUtil.setChildData(self.host, 'speed', speed)

    def getSpeed(self, progress=None):
        """Return the speed of this Location's host. This will be loaded
           from the configuration database if it has already been calculated
           and we're not forcing a retest, otherwise it will be tested
           using testSpeed().
           """
        needTest = 0

        # If we have no <speed> tag in the host, we definitely need to test
        if not self.host.getElementsByTagName('speed'):
            needTest = 1

        # Are we being forced to retest?
        if self.config.eval("invocation/option[@name='retestMirrors']/text()"):
            # We only want to retest all the mirrors once each
            if not hasattr(self, 'retested'):
                needTest = 1
                self.retested = 1

        if needTest:
            self.testSpeed()
            if progress:
                # Get a little info to make a more useful progress report
                server = urlparse.urlparse(self.absoluteURI)[1]
                speed = float(PGBuild.XMLUtil.getChildData(self.host, 'speed'))
                progress.report("tested", "%7.2f KB/s - %s" % (speed/1000, server))
        return float(PGBuild.XMLUtil.getChildData(self.host, 'speed'))


def expand(config, tags):
    """Recursively resolve a list of <a> tags that may contain
       references to sites. Returns a list of Locations.
       """
    results = []
    for tag in tags:
        sites = tag.getElementsByTagName('site')
        href = tag.attributes['href'].value
        if len(sites) > 1:
            raise PGBuild.Errors.ConfigError("Found an <a> tag with more than one <site> tag inside")
        if sites:
            # This is a site-relative link. Find the site and expand it.
            resolvedResults = expand(config, findSite(config, sites[0]))
            # Join our path onto the end of each result as we append it to our results
            for resolvedResult in resolvedResults:
                results.append(Location(config, urljoin(resolvedResult.absoluteURI, href),
                                        resolvedResult.host))
        else:
            # This is an absolute link- just add it to our results
            results.append(Location(config, href, tag))
    return results
            

def resolve(config, tags, progress=None):
    """Given a list of <a> tags, expand them into absolute URIs
       and pick the fastest mirror. Returns a Location.
       """
    mirrors = expand(config, tags)
    def speedSort(a,b):
        return cmp(b.getSpeed(progress),a.getSpeed(progress))
    mirrors.sort(speedSort)
    return mirrors[0]


### The End ###
        
    
