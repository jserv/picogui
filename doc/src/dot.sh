#!/bin/sh
# $Id: dot.sh,v 1.1 2000/10/02 22:39:01 micahjd Exp $
#
# Just a short wrapper to turn a .dot file into
# various useful file formats. 
# Assumes input from stdin.
#
# Usage:
#  dot.sh (dot|neato) <basename>
#

$1 -Tps -o ps/$2.ps
# Natural size
convert ps/$2.ps html/$2.jpeg
# Screen-size (could be bigger or smaller)
convert -geometry 1024x768 ps/$2.ps html/$2.1024.jpeg
# Thumbnail
convert -geometry 400x300 ps/$2.ps html/$2.400.jpeg
