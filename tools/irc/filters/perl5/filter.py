#!/usr/bin/env python
import sys, email, smtplib, re
from StringIO import StringIO

returnAddress = "perl_commits@picogui.org"
toAddress = "commits@picogui.org"
projectName = "perl"
logFile = "/home/perl_commits/mail.log"

message = email.message_from_file(sys.stdin)
body = StringIO(message.get_payload())

# Log the raw message for debugging
open(logFile, "a").write(str(message))

# First line gives us changeset number and author:
changeFields = body.readline().strip().split(" ")
csNum = changeFields[1]
author = changeFields[3]

# Read the log until we hit a heading (line that doesn't start with whitespace)
log = ""
body.readline()
while True:
    line = body.readline()
    if not re.match('\s', line[0]):
        break
    log += line.strip() + " "

ciaMessage = "Changeset %s by {green}%s{normal}: %s" % (csNum, author, log)

s = smtplib.SMTP()
s.connect()
s.sendmail(returnAddress, toAddress,
           "From: %s\nTo: %s\nSubject: Announce %s\n\n%s" % \
           (returnAddress, toAddress, projectName, ciaMessage))
s.close()
