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
_svn_id = "$Id$"

def findSite(ctx, name):
    """Look up a site name, returning a list of <a> tags.
       This can be passed a <site> tag or a simple name string.
       """
    if type(name) != str:
        name = name.attributes['name'].value
    return ctx.config.xpath("sites/site[@name='%s']/a" % name)


def urljoin(a, b):
    """Our own version of urlparse.urljoin(). Forces the first argument
       to be interpreted as a directory name, to give semantics closer
       to os.path.join()
       """
    if a[-1] != '/':
        a = a + '/'
    import urlparse
    return urlparse.urljoin(a,b)


class Location(object):
    """Encapsulate one download location. Stores the aboslute URI for
       that location and reference to the <a> tag for its host.
       Includes methods to perform speed testing.
       """
    def __init__(self, absoluteURI, hostTag):
        self.absoluteURI = absoluteURI
        self.hostTag = hostTag

    def append(self, paths):
        """Append a list of paths to the URI. This is where paths appended via
           <packagegroup> tags eventually end up.
           """
        for path in paths:
            self.absoluteURI = urljoin(self.absoluteURI, path)

    def testSpeed(self):
	"""Try to retrieve the first few kilobytes of the URL, measuring
           the speed. If the site is down, set the speed to zero.
           """
        try:
            import urllib2, time
            bytes = 0
            startTime = time.time()
            url = urllib2.urlopen(self.absoluteURI)
            bytes += len(url.read(64000))
            url.close()
            dlTime = time.time() - startTime
            speed = bytes / dlTime
            
            # If for any reason other than a user interrupt we can't test this mirror,
            # set its speed to zero so we won't use it given a choice. This includes 404
            # errors and other indications that the mirror isn't even functional.
        except KeyboardInterrupt:
            raise
        except:
            speed = 0
        PGBuild.XMLUtil.setChildData(self.hostTag, 'speed', speed)

    def getSpeed(self, ctx):
        """Return the speed of this Location's host. This will be loaded
           from the configuration database if it has already been calculated
           and we're not forcing a retest, otherwise it will be tested
           using testSpeed().
           """
        needTest = 0

        # If we have no <speed> tag in the host, we definitely need to test
        if not self.hostTag.getElementsByTagName('speed'):
            needTest = 1

        # Are we being forced to retest?
        if ctx.config.eval("invocation/option[@name='retestMirrors']/text()"):
            # We only want to retest all the mirrors once each
            if not hasattr(self, 'retested'):
                needTest = 1
                self.retested = 1

        if needTest:
            import urlparse
            progress.showTaskHeading()
            self.testSpeed()
            server = urlparse.urlparse(self.absoluteURI)[1]
            speed = float(PGBuild.XMLUtil.getChildData(self.hostTag, 'speed'))
            if speed > 0:
                ctx.progress.report("tested", "%7.2f KB/s from %s" % (speed/1000, server))
            else:
                ctx.progress.warning("unable to download from %s" % server)
        return float(PGBuild.XMLUtil.getChildData(self.hostTag, 'speed'))


def expand(ctx, tags):
    """Recursively resolve a list of <a> tags that may contain
       references to sites. Returns a list of Locations.
       """
    results = []
    for tag in tags:
        sites = tag.getElementsByTagName('site')
        href = tag.attributes['href'].value
        if len(sites) > 1:
            import PGBuild.Errors
            raise PGBuild.Errors.ConfigError("Found an <a> tag with more than one <site> tag inside")
        if sites:
            # This is a site-relative link. Find the site and expand it.
            resolvedResults = expand(ctx, findSite(ctx, sites[0]))
            # Join our path onto the end of each result as we append it to our results
            for resolvedResult in resolvedResults:
                results.append(Location(urljoin(resolvedResult.absoluteURI, href),
                                        resolvedResult.hostTag))
        else:
            # This is an absolute link- just add it to our results
            results.append(Location(href, tag))
    return results
            

def resolve(ctx, tags, appendPaths=[]):
    """Given a list of <a> tags, expand them into absolute URIs
       and pick the fastest mirror. Returns a Location.
       appendPaths gives a list of paths to be appended to each URI before use.
       """
    mirrors = expand(ctx, tags)
    for mirror in mirrors:
        mirror.append(appendPaths)
    def speedSort(a,b):
        return cmp(b.getSpeed(progress),a.getSpeed(progress))
    mirrors.sort(speedSort)
    return mirrors[0]


### The End ###
        
    
