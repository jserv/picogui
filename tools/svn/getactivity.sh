#!/bin/bash
#
# Generate statistics on Subversion repository activity.
# Outputs lines of the form:
#    revision unix-time committer changed-lines
#
# This requires access to the repository itself, not
# a URL, as it uses svnlook.
#

REPOSITORY=/home/svn/picogui
OUTPUT_FILE=activity.log

FIRST_REVISION=1
LAST_REVISION=`svnlook youngest $REPOSITORY`

rm -f $OUTPUT_FILE

for ((rev=FIRST_REVISION;rev<=LAST_REVISION;rev++)); do
    # Calculate the number of lines added and deleted in this revision
    lines=`svnlook diff -r $rev $REPOSITORY | egrep '^[+\-][^+\-]' | wc -l`

    # Get the committer name
    committer=`svnlook author -r $rev $REPOSITORY`

    # Get the date, transmogrified into UNIX time
    date=`svnlook date -r $rev $REPOSITORY | date -f - +%s`

    echo $rev $date $committer $lines
    echo $rev $date $committer $lines >> $OUTPUT_FILE
done