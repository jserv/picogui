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
import urlparse


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
        PGBuild.XMLUtil.setChildData(self.host, 'speed', 1234)

    def getSpeed(self):
        """Return the speed of this Location's host. This will be loaded
           from the configuration database if it has already been calculated
           and we're not forcing a retest, otherwise it will be tested
           using testSpeed().
           """
        # If we have no <speed> tag in the host, or we're being
        # forced to retest, test the speed
        if self.config.eval("invocation/option[@name='retestMirrors']/text()") or \
           not self.host.getElementsByTagName('speed'):
            self.testSpeed()
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
            

def resolve(config, progress, tags):
    """Given a list of <a> tags, expand them into absolute URIs
       and pick the fastest mirror. Returns a Location.
       """
    mirrors = expand(config, tags)
    def speedSort(a,b):
        return a.getSpeed() <> b.getSpeed()
    mirrors.sort(speedSort)
    return mirrors[-1]


### The End ###
        
    
