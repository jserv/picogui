#!/usr/bin/env python
#
# CGI to display disk status on Navi
# -- Micah Dowty <micah@picogui.org>
#

import navi_cgi, os, re


class MountPoint:
    def __init__(self, device, used, available, path):
        """All sizes specified in 1K blocks, as returned by 'df'"""
        self.device = device
        self.used = used
        self.available = available
        self.path = path


def formatSize(size):
    """Reformat a size specified in 1K blocks into something more human-readable"""
    if size < 768:
        return "%0.2f kB" % size
    elif size < 786432:
        return "%0.2f MB" % (size / 1024.0)
    elif size < 805306368:
        return "%0.2f GB" % (size / 1048576.0)
    else:
        return "%0.2f TB" % (size / 1073741824.0)


def getMountPoints():
    """This makes a call to 'df' and converts the output to a list of MountPoints"""
    f = os.popen('df')
    f.readline()
    mounts = []
    for line in f.readlines():
        (device, total, used, available, percent, path, whitespace) = re.split("\s+", line)
        mounts.append(MountPoint(device, long(used), long(available), path))
    f.close()
    return mounts

    
class DiskChart:
    """Base class for a chart in the 'charts' section"""
    def __init__(self, page):
        self.page = page

    def output(self, write):
        write('<table class="chart"><tr class="chart"><td class="chartBody">')
        self.outputBody(write)
        write('</td><td class="chartLegend">')
        self.outputLegend(write)
        write('</td></tr></table>')
        
    def outputBody(self, write):
        write('<img width="%s" height="%s" src="%s"/>' %
              (self.page.width, self.page.height, self.page.linkURL({
            'image': 'disk_chart',
            'percentsUsed': "+".join(map(str, self.getPercentsUsed())),
            'percentsFree': "+".join(map(str, self.getPercentsFree())),
            })))
        
    def outputLegend(self, write):
        write('%0.2f%%<br/><br/>' % self.getTotalPercentUsed())
        write('%s total<br/>' % self.getTotal())
        write('%s used<br/>' % self.getUsed())
        write('%s available<br/>' % self.getAvailable())

    def getTotalPercentUsed(self):
        total = 0
        for percent in self.getPercentsUsed():
            total += percent
        return total


class MountPointChart(DiskChart):
    """Chart to display information about one mount point"""
    def __init__(self, page, mount):
        DiskChart.__init__(self, page)
        self.mount = mount

    def getName(self):
        return self.mount.path

    def getPercentsUsed(self):
        return [self.mount.used * 100.0 / (self.mount.used + self.mount.available)]

    def getPercentsFree(self):
        return [100 - self.getPercentsUsed()[0]]

    def getTotal(self):
        return formatSize(self.mount.used + self.mount.available)

    def getUsed(self):
        return formatSize(self.mount.used)

    def getAvailable(self):
        return formatSize(self.mount.available)


class OverviewChart(DiskChart):
    """Chart to display totals for all mount points"""
    def __init__(self, page, mounts):
        DiskChart.__init__(self, page)
        self.mounts = mounts
        self.total = 0
        self.totalUsed = 0
        self.totalAvailable = 0
        for mount in mounts:
            self.total += mount.used + mount.available
            self.totalUsed += mount.used
            self.totalAvailable += mount.available

    def getName(self):
        return "Overview"

    def getPercentsUsed(self):
        percents = []
        for mount in self.mounts:
            percents.append(mount.used * 100.0 / self.total)
        return percents

    def getPercentsFree(self):
        percents = []
        for mount in self.mounts:
            percents.append(mount.available * 100.0 / self.total)
        return percents

    def getTotal(self):
        return formatSize(self.total)

    def getUsed(self):
        return formatSize(self.totalUsed)

    def getAvailable(self):
        return formatSize(self.totalAvailable)


class DiskPage(navi_cgi.NaviPage):
    plainTitle = "Navi disk status"
    htmlTitle = '<a href="/">Navi</a> disk status'
    allSections = ['charts', 'uptime', 'mounts', 'raid']
    parameters = navi_cgi.NaviPage.parameters + [ 
        'percentsUsed',
        'percentsFree',
       ('chartsPerLine', int),
        ]
    width = 128
    height = 128
    chartsPerLine = 3

    def runCommand(self, write, command):
        f = os.popen(command)
        output = f.read()
        f.close()
        output = re.sub("&", "&amp;", output)
        output = re.sub("<", "&lt;", output)
        output = re.sub("\n", "<br/>", output)
        output = re.sub("\s", "&nbsp;", output)
        write(output)

    def image_disk_chart(self):
        slices = []
        for percent in self.percentsUsed.split(" "):
            slices.append((float(percent) / 100.0, 0xA0A0FF))
        for percent in self.percentsFree.split(" "):
            slices.append((float(percent) / 100.0, 0xA0FFA0))
        return navi_cgi.pieChart(self.width, self.height, slices)

    def section_charts(self, write):
        self.begin_section(write, "charts", "Charts")

        mounts = getMountPoints()

        # Overview chart
        charts = [OverviewChart(self, mounts)]

        # Create charts for each mount point
        for mount in mounts:
            charts.append(MountPointChart(self, mount))
            
        # Split up the charts into separate tables to obey chartsPerLine
        while charts:
            lineContents = charts[:self.chartsPerLine]
            charts = charts[self.chartsPerLine:]

            # Headings for all charts on this line
            write('<table><tr>')
            for chart in lineContents:
                write('<th>%s</th>' % chart.getName())
            write('</tr><tr>')

            # Charts and legends for this line
            for chart in lineContents:
                write('<td>')
                chart.output(write)
                write('</td>')
            write('</tr></table>')
            
        self.end_section(write)
                
    def section_uptime(self, write):
        self.begin_section(write, "uptime", "Uptime", "monoBox")
        self.runCommand(write, 'uptime')
        self.end_section(write)

    def section_mounts(self, write):
        self.begin_section(write, "mounts", "Mounts", "monoBox")
        self.runCommand(write, 'df -h')
        self.end_section(write)

    def section_raid(self, write):
        self.begin_section(write, "raid", "RAID", "monoBox")
        self.runCommand(write, 'cat /proc/mdstat')
        self.end_section(write)

if __name__ == '__main__':
    DiskPage().run()

