#!/bin/sh
# $Id$
#
# Just a short wrapper to turn a .dot file into
# various useful file formats. 
# Assumes input from stdin.
#
# Usage:
#  dot.sh (dot|neato) <basename>
#

echo Generating postscript...

$1 -Tps -o images/$2.ps

echo Generating png...

# Natural size
convert images/$2.ps images/$2.png

echo Generating screen-sized png...

# Screen-size (could be bigger or smaller)
convert -geometry 1024x768 images/$2.ps images/$2.1024.png

echo Generating thumbnail png...

# Thumbnail
convert -geometry 400x300 images/$2.ps images/$2.400.png
