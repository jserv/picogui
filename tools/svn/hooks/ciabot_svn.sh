#!/bin/sh
#
# CIA bot notification script for Subversion repositories.
# -- Micah Dowty <micah@picogui.org>
#
# See http://navi.picogui.org/svn/picogui/trunk/tools/irc/cia.html
# for more information on what the CIA bot is and how it works.
#
# This script should be called from your repository's post-commit
# hook with the repository and revision as arguments. For example,
# you could copy this script into your repository's "hooks" directory
# and add something like the following to the "post-commit" script,
# also in the repository's "hooks" directory:
#
#   REPOS="$1"
#   REV="$2"
#   $REPOS/hooks/ciabot_svn.sh "$REPOS" "$REV"&
#
# Note that this version of the script requires python. If you can't
# get python on your subversion server, you'll need to remove
# or modify the code below that finds the parent directory of all changes.
#
##### There are some parameters for this script that you can customize:

project_name="picogui"
return_address="micah@picogui.org"
log_message_lines="6"
sendmail_command="/usr/sbin/sendmail -t"

##### Below this line you shouldn't have to change anything unless you
##### want more extensive customization

# Script arguments
REPOS="$1"
REV="$2"

# The address CIA lives at
cia_address="commits@picogui.org"

# Use svnlook and a python oneliner to find the base directory of all changes
basedir=`svnlook dirs-changed -r "$REV" "$REPOS" | python -c \
         'import os,sys;print os.path.normpath(os.path.commonprefix(sys.stdin.readlines())[:-1])'`

# Get the commit author using svnlook
author=`svnlook author -r "$REV" "$REPOS"`

# Now compose a commit message using the data determined above
# and the first few lines of the log message, emailing it to CIA.
(
   echo "From: $return_address"
   echo "To: $cia_address"
   echo "Content-Type: text/plain;"
   echo "Subject: Announce $project_name"
   echo
   echo -n "{aqua}$basedir{normal} r$REV {green}$author{normal}: "
   svnlook log -r "$REV" "$REPOS" | head -n $log_message_lines
) | $sendmail_command

