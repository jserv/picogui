#!/usr/bin/env python
#
# CGI to display disk status on Navi
# -- Micah Dowty <micah@picogui.org>
#

import navi_cgi, os, re

class DiskPage(navi_cgi.NaviPage):
    plainTitle = "Navi disk status"
    htmlTitle = '<a href="/">Navi</a> disk status'
    allSections = ['uptime', 'mounts', 'raid']

    def runCommand(self, write, command):
        f = os.popen(command)
        output = f.read()
        f.close()
        write(re.sub("\s", "&nbsp;", re.sub("\n", "<br/>", output)))
    
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

