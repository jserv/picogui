#!/usr/bin/env python
#
# Called from a crontab, this sends stats back to the CIA bot
#

import smtplib
import cia_statreader

def sendMessage(message, project="stats"):
    """Send a message back to the bot. For now this uses email, so that
       these messages will be logged, and so this script isn't dependent
       on the socket format the email client and bot itself communicate with.
       """
    fromAddr = "statmailer@picogui.org"
    toAddr = "commits@picogui.org"
    message = "From: %s\r\nTo: %s\r\nSubject: Announce %s\r\n\r\n%s\r\n" \
              % (fromAddr, toAddr, project, message)
    server = smtplib.SMTP('localhost')
    server.sendmail(fromAddr, toAddr, message)
    server.quit()
    

def getStatLine(subdir):
    message = "%s:" % cia_statreader.statHeadings[subdir]
    todayStats = []
    # Build a list of the stats first so we can sort them
    for project in cia_statreader.projects:
        for sub in cia_statreader.statSubdirs:
            if subdir == sub:
                count = cia_statreader.projectCounts[project][subdir]
                if count > 0:
                    todayStats.append((count, project))
    def sortReverse(a,b):
        return cmp(b,a)
    todayStats.sort(sortReverse)
    for stat in todayStats:
        message += " %s(%d)" % (stat[1], stat[0])     
    return message


def getStatMessage():
    message = ""
    for subdir in cia_statreader.statSubdirs:
        # We do this right after deleting the daily stats, so skip daily
        if subdir != 'daily':
            message += getStatLine(subdir) + "\n"
    return message

if __name__ == '__main__':
    sendMessage(getStatMessage())
