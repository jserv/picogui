#!/bin/sh

# script to parse gnu emacs commit messages
msg=`cat`
logmsg_line=$( echo "$msg" | grep -n "Log message:" | sed 's/:.*/' )
total_lines=$( echo `echo "$msg" | wc -l` )
logmsg=$( echo "$msg" | tail -n $[ $total_lines - $logmsg_line ] )
logmsg=$( echo "$logmsg" | head -n `echo "$logmes" | grep -n "" | sed 's/:.*/'` )

# who committed?
who=$( echo "$msg" | grep "Changes by:" | sed 's|<\(.*\)>|\1|' )

cia_message="From: emacs-commits@navi.picogui.org
To: commits@picogui.org
Subject: Announce emacs

commit by {lightblue}$who{normal}: $logmsg"

echo "$cia_message" | /usr/sbin/sendmail -t
