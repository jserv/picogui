#!/usr/bin/env python
#
# Simple CGI script to generate a page with per-project
# message counts generated by the CIA bot.
# -- Micah Dowty <micah@picogui.org>
#

import navi_cgi, cia_statreader
import os, math, re, time, cgi

def convertDuration(t):
    """Convert a duration in seconds to other units depending on its magnitude"""
    if not t:
        t = ''
    elif t > 172800:
        t = "%.1fd" % (t / 86400)
    elif t > 7200:
        t = "%.1fh" % (t / 3600)
    elif t > 120:
        t = "%.1fm" % (t / 60)
    else:
        t = "%.1fs" % t
    return t

class StatPage(navi_cgi.NaviPage):
    plainTitle = "CIA bot statistics"
    htmlTitle = '<a href="http://navi.picogui.org/svn/picogui/trunk/tools/irc/cia.html">CIA bot</a> statistics'
    subtitle = "Because SF stats weren't pointless enough"
    allSections = ['table', 'totals', 'channels', 'recent']
    footer = navi_cgi.NaviPage.footer + '<a href="http://freenode.org"><img src="/images/web/freenode.png" width="137" height="39" alt="freenode"/></a>'
    sort = 'forever_D'
    recentLines = 20
    parameters = navi_cgi.NaviPage.parameters + [
        'sort',
        ('recentLines', int),
        ]
    
    def section_table(self, write):
        self.begin_section(write, "table", "Number of commits posted per project")

        (sortKey, sortDirection) = self.sort.split("_")
        
        # Project table heading
        write("<table><tr>")
        for heading in ('project','mtbc') + cia_statreader.statSubdirs:
            if heading == sortKey:
                # This is the current sort key. Embolden the title and link to the opposite sort direction.
                boldOpen = "<b>"
                boldClose = "</b>"
                if sortDirection == "A":
                    url = self.linkURL({'sort': "%s_D" % heading})
                else:
                    url = self.linkURL({'sort': "%s_A" % heading})
            else:
                # This isn't the current sort key. Default to ascending sort.
                boldOpen = boldClose = ""
                url = self.linkURL({'sort': "%s_A" % heading})

            write('<th><a href="%s">%s%s%s</a></th>' %
                  (url, boldOpen, cia_statreader.statHeadings[heading], boldClose))
        write("</tr>")

        # Save the highest number for each column
        columnMaxima = [0] * len(cia_statreader.statSubdirs)
        for project in cia_statreader.projects:
            for i in xrange(len(cia_statreader.statSubdirs)):
                if cia_statreader.projectCounts[project][cia_statreader.statSubdirs[i]] > columnMaxima[i]:
                    columnMaxima[i] = cia_statreader.projectCounts[project][cia_statreader.statSubdirs[i]]

        # Resort the project list according to the current key and direction
        def getKey(project):
            if sortKey == "project":
                return project
            if sortKey == "mtbc":
                mtbc = cia_statreader.projectMTBC[project]
                if not mtbc:
                    # For sorting purposes, an MTBC of None should appear larger than all others
                    mtbc = 1e+1000   # Infinity, or so...
                return mtbc
            return cia_statreader.projectCounts[project][sortKey]
        def projectSort(a,b):
            if sortDirection == "A":
                return cmp(getKey(a),getKey(b))
            else:
                return cmp(getKey(b),getKey(a))
        cia_statreader.projects.sort(projectSort)

        # Project table contents
        for project in cia_statreader.projects:
            linkOpen = ''
            linkClose = ''
            url = cia_statreader.projectURL[project]
            if url:
                linkOpen = '<a href="%s">' % re.sub('"','', url)
                linkClose = '</a>'
            write("<tr><td>%s%s%s</td><td>%s</td>" % (linkOpen, project, linkClose, convertDuration(cia_statreader.projectMTBC[project])))
            for statIndex in xrange(len(cia_statreader.statSubdirs)):
                count = cia_statreader.projectCounts[project][cia_statreader.statSubdirs[statIndex]]
                if count:
                    # Get a fraction of this count compared to the highest in the column
                    logMax = math.log(columnMaxima[statIndex])
                    if logMax > 0:
                        fraction = math.log(count) / logMax
                    else:
                        fraction = 1.0
                    # Multiply by the desired maximum bar length in EMs, add the minimum bar padding
                    width = fraction * 4 + 0.2
                    # A stupid trick for making bargraph thingies
                    write('<td><span class="bargraph" style="padding: 0em %.4fem;">%s</span></td>' % (width, count))
                else:
                    write("<td></td>")
            write("</tr>")
            
        write("""
           </table>
           (graph is logarithmic. MTBC == Mean Time Between Commits)
           """)
        self.end_section(write)

    def section_totals(self, write):
        self.begin_section(write, "totals", "Totals")
        write("<ul>")            
        write("<li>%d channels</li>" % len(cia_statreader.channels))
        write("<li>%d projects</li>" % len(cia_statreader.projects))
        msgTotal = 0
        for projCount in cia_statreader.projectCounts.values():
            msgTotal += projCount[cia_statreader.statSubdirs[0]]
        write("<li>%d messages</li>" % msgTotal)
        write("<li>overall MTBC: %s</li>" % convertDuration(cia_statreader.totalMTBC))
        write("</ul>")
        self.end_section(write)

    def section_channels(self, write):
        self.begin_section(write, "channels", "Channels")
        for channel in cia_statreader.channels:
            write("#%s " % channel)
        self.end_section(write)
        
    def section_recent(self, write):
        self.begin_section(write, "recent", "Most recent commits", "terminalBox")
        write("<ul>")
        for command in cia_statreader.readLatestCommands(self.recentLines):
            if command[0] == "Announce":
                projectName = command[1]
                messageLine = cia_statreader.htmlifyColorTags(command[2])
                write("<li><b>%s</b>: %s</li>" % (projectName, messageLine))
        write("</ul>")
        self.end_section(write)

if __name__ == '__main__':
    StatPage().run()
