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


def expand(config, tags):
    """Recursively resolve a list of <a> tags
       that may contain references to sites into a list of
       (absolute URI, innermost <a> tag) tuples.
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
                results.append((urljoin(resolvedResult[0], href), resolvedResult[1]))
        else:
            # This is an absolute link- just add it to our results
            results.append((href, tag))
    return results
            

def resolve(config, progress, tags):
    """Given a list of <a> tags, expand them into absolute URIs
       and pick the fastest mirror. Returns an
       (Absolute URI, innermost <a> tag) tuple.
       """

    # xxx
    return expand(config, tags)

### The End ###
        
    
