#!/usr/bin/env python
#
# CGI to display disk status on Navi
# -- Micah Dowty <micah@picogui.org>
#

import navi_cgi, os, re

class DiskPage(navi_cgi.NaviPage):
    plainTitle = "Navi disk status"
    htmlTitle = '<a href="/">Navi</a> disk status'
    allSections = ['charts', 'uptime', 'mounts', 'raid']
    parameters = navi_cgi.NaviPage.parameters + [
        ('percent', float),
        ]
    width = 128
    height = 128

    def runCommand(self, write, command):
        f = os.popen(command)
        output = f.read()
        f.close()
        output = re.sub("&", "&amp;", output)
        output = re.sub("<", "&lt;", output)
        output = re.sub("\n", "<br/>", output)
        output = re.sub("\s", "&nbsp;", output)
        write(output)

    def image_usage_pie(self):
        slices = [
            (self.percent / 100.0, 0xA0A0FF),
            (None, 0xA0FFA0),
            ]
        return navi_cgi.pieChart(self.width, self.height, slices)

    def section_charts(self, write):
        self.begin_section(write, "charts", "Charts")

        # Read data about all mounts from the 'df' command
        f = os.popen('df -h')
        f.readline()
        mounts = {}
        for line in f.readlines():
            (device, total, used, available, percent, mount, whitespace) = re.split("\s+", line)
            mounts[mount] = (percent[:-1], total, used, available)
        f.close()

        for mount in mounts:
            write('<div class="chartBox">')
            write('<div class="chartHeading">%s</div>' % mount)
            write('<div class="chartBody">')
            write('<span class="chart">')

            # Invoke our image_usage_pie() function to generate the chart
            write('<img width="%s" height="%s" src="%s"/>' %
                  (self.width, self.height, self.linkURL({
                'image': 'usage_pie',
                'percent': mounts[mount][0]
                })))
            
            # Add some numbers
            write('</span><span class="chartLegend">')
            write('%s%%<br/><br/>' % mounts[mount][0])
            write('%s total <br/>' % mounts[mount][1])
            write('%s used <br/>' % mounts[mount][2])
            write('%s available <br/>' % mounts[mount][3])
            write('</span></div></div>')
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

