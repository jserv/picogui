#
# Simple disorganized python module for processing CIA stats
#

import os

statDir = "/home/commits/stats"
mtbcSubdir = 'mtbc'
channelFile = "/home/commits/channels.list"
commandLog = "/home/commits/commands.log"

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
    'mtbc': 'MTBC',
    }

# Projects we want to hide from stat lists
hiddenProjects = ('stats', 'test')

def loadInt(f):
    f = open(f)
    count = int(f.read().strip())
    f.close()
    return count

def readStats():
    """Stats are automatically read into the module on import,
       this can be called separately to refresh them.
       """
    global projects, channels, projectCounts, projectMTBC, totalMTBC

    # Add all projects that we aren't excempting from stats
    projects = []
    for project in os.listdir(os.path.join(statDir, statSubdirs[0])):
        if not project in hiddenProjects:
            projects.append(project)

    # Yucky hack to strip off "#"
    channels = open(channelFile).read().strip().split("\n")
    channels.sort()

    projectCounts = {}
    for project in projects:
        # Build a map of counts indexed by subdirectory
        counts = {}
        for subdir in statSubdirs:
            try:
                counts[subdir] = loadInt(os.path.join(statDir, subdir, project))
            except IOError:
                counts[subdir] = 0
        projectCounts[project] = counts

    # Sort the project list by the 'forever' count, descending
    def countSort(a,b):
        return cmp(projectCounts[b][statSubdirs[0]],
                   projectCounts[a][statSubdirs[0]])
    projects.sort(countSort)

    # Calculate the Mean Time Between Commits, total and per-project
    totalSamples = 0
    totalTime = 0
    projectMTBC = {}
    for project in ['commits'] + projects:
        try:
            projectSamples = loadInt(os.path.join(statDir, mtbcSubdir, project + '.numSamples'))
            projectTime = loadInt(os.path.join(statDir, mtbcSubdir, project + '.totalTime'))
            projectMTBC[project] = projectTime * 1.0 / projectSamples
            totalSamples += projectSamples
            totalTime += projectTime
        except IOError:
            projectMTBC[project] = None
    totalMTBC = projectMTBC['commits']

def readLatestCommands(n=20):
    """Read the n latest commands, returning a list of (command, project, message) tuples"""
    f = open(commandLog)
    
    # Go to the end, and read backwards for n+1 newlines
    f.seek(-1,2)
    for i in xrange(n+1):
        while f.read(1) != "\n":
            f.seek(-2,1)
        f.seek(-2,1)
    f.readline()
            
    # Parse up some lines
    results = []
    for i in xrange(n):
        line = f.readline().strip()
        results.append(line.split(" ", 2))
    f.close()
    return results

readStats()
