#
# Simple python module for processing CIA stats
#

import os

statDir = "/home/commits/stats"
channelFile = "/home/commits/channels.list"

# List out the subdirs explicitly so we can set the order-
# the first one here is used as the sort key, and for definatively
# listing the available projects.
statSubdirs = ('forever', 'monthly', 'weekly', 'end-of-day', 'daily')

# Mapping between subdirectory names and table headings
statHeadings = {
    'project': 'project',
    'forever': 'forever',
    'monthly': 'this month',
    'weekly': 'this week',
    'end-of-day': 'yesterday',
    'daily': 'today',
    }

def readStats():
    """Stats are automatically read into the module on import,
       this can be called separately to refresh them.
       """
    global projects, channels, projectCounts
    
    projects = os.listdir(os.path.join(statDir, statSubdirs[0]))

    # Yucky hack to strip off "#"
    channels = []
    for channel in open(channelFile).read().strip().split("\n"):
        if channel[0] == '#':
            channels.append(channel[1:])
        else:
            channels.append(channel)

    projectCounts = {}
    for project in projects:
        # Build a map of counts indexed by subdirectory
        counts = {}
        for subdir in statSubdirs:
            try:
                counts[subdir] = int(open(os.path.join(statDir, subdir, project)).read().strip())
            except IOError:
                counts[subdir] = 0
        projectCounts[project] = counts

    # Sort the project list by the 'forever' count, descending
    def countSort(a,b):
        return cmp(projectCounts[b][statSubdirs[0]],
                   projectCounts[a][statSubdirs[0]])
    projects.sort(countSort)

readStats()
