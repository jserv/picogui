#
# Simple disorganized python module for processing CIA stats
#

import os, glob, time

baseDir = "/home/commits"
statDir = os.path.join(baseDir, "stats")
urlDir = os.path.join(baseDir, "urls")
mtbcSubdir = 'mtbc'
channelFile = os.path.join(baseDir, "channels.list")
commandLog = os.path.join(baseDir, "commands.log")

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

def loadInt(f, default=0):
    try:
        f = open(f)
        count = int(f.read().strip())
        f.close()
        return count
    except IOError:
        return default

def readStats():
    """Stats are automatically read into the module on import,
       this can be called separately to refresh them.
       """
    global projects, channels, projectCounts, projectMTBC, totalMTBC, projectURL

    # Add all projects that we aren't excempting from stats
    projects = []
    for project in os.listdir(os.path.join(statDir, statSubdirs[0])):
        if not project in hiddenProjects:
            projects.append(project)

    channels = []
    for channel in open(channelFile).read().split("\n"):
        channel = channel.strip()
	if channel:
	    channels.append(channel)
    channels.sort()

    projectCounts = {}
    for project in projects:
        # Build a map of counts indexed by subdirectory
        counts = {}
        for subdir in statSubdirs:
            counts[subdir] = loadInt(os.path.join(statDir, subdir, project))
        projectCounts[project] = counts

    # Sort the project list by the 'forever' count, descending
    def countSort(a,b):
        return cmp(projectCounts[b][statSubdirs[0]],
                   projectCounts[a][statSubdirs[0]])
    projects.sort(countSort)

    # Calculate the Mean Time Between Commits, total and per-project
    projectMTBC = {}
    now = time.time()
    for project in ['commits'] + projects:
        projectLastTime = loadInt(os.path.join(statDir, mtbcSubdir, project + '.lastTime'))
        projectSamples = loadInt(os.path.join(statDir, mtbcSubdir, project + '.numSamples'))
        projectTime = loadInt(os.path.join(statDir, mtbcSubdir, project + '.totalTime'))

        if projectSamples:
            mtbc = projectTime * 1.0 / projectSamples
            # If it's been longer than the MTBC since the last commit, weigh this in with the calculated MTBC
            if now - projectLastTime > mtbc:
                mtbc = (projectTime + (now - projectLastTime)) * 1.0 / (projectSamples + 1)
        else:
            # We don't have enough samples to calculate an MTBC. If we have one sample, just give the time
            # since that sample was taken.
            if projectLastTime:
                mtbc = now - projectLastTime
            else:
                mtbc = None
        projectMTBC[project] = mtbc

    totalMTBC = projectMTBC['commits']

    # Get project URLs
    projectURL = {}
    for project in projects:
        try:
            projectURL[project] = open(os.path.join(urlDir, project)).read().strip()
        except IOError:
            projectURL[project] = None

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

def htmlifyColorTags(message):
    """Utility to properly convert message color tags into <span> tags with class attributes."""
    # We have to parse the message from the beginning and keep a list of
    # active colors, since our color tag format allows things like {red}ecky{green}foo{normal}

    insideTags = []
    outputMessage = ""
    import irc_colors, re, copy
    message = re.sub("<", "&lt;", message)

    class ColorState:
        fgColor = None
        bgColor = None
        bold = False
        underline = False

    parsedState = ColorState()
    htmlState = ColorState()
    
    while message:
        nextSquiggly = message.find("{")
        if nextSquiggly != 0:
            # Normal text.
            
            if htmlState != parsedState:
                # Now figure out how to change our HTML state to match our parsed color tag state.
                # We might be smarter about this later, for now just close all the tags we're in
                # and open new ones.
                while insideTags:
                    outputMessage += "</%s>" % insideTags.pop()
                if parsedState.bgColor:
                    outputMessage += '<span class="bgColor-%s">' % re.sub(" ", "-", parsedState.bgColor)
                    insideTags.append('span')
                if parsedState.fgColor:
                    outputMessage += '<span class="fgColor-%s">' % re.sub(" ", "-", parsedState.fgColor)
                    insideTags.append('span')
                if parsedState.bold:
                    outputMessage += '<b>'
                    insideTags.append('b')
                if parsedState.underline:
                    outputMessage += '<u>'
                    insideTags.append('u')
                htmlState = copy.deepcopy(parsedState)

            if nextSquiggly > 0:
                outputMessage += message[:nextSquiggly]
                message = message[nextSquiggly:]
            else:
                outputMessage += message
                message = ''
        else:
            tagEnd = message.find("}")
            if tagEnd < 1:
                # Unclosed tag, just dump the rest of the message
                return outputMessage + message
            tag = message[1:tagEnd]

            # Chomp up the color tag
            message = message[len(tag)+2:]

            # We have a tag, see if it's something we recognize
            if tag == "bold":
                parsedState.bold = True
            elif tag == "underline":
                parsedState.underline = True
            elif tag == "reverse":
                (parsedState.fgColor, parsedState.bgColor) = (parsedState.bgColor, parsedState.fgColor)
            elif tag == "normal":
                parsedState = ColorState()
            elif tag in irc_colors.COLORS.keys():
                parsedState.fgColor = tag
            else:
                # Unrecognized tag- output it unmodified
                outputMessage += "{" + tag + "}"
                continue

    # Make sure all our tags are closed
    while insideTags:
        outputMessage += "</%s>" % insideTags.pop()
    return outputMessage

readStats()
