#!/bin/sh
convert -geometry 160x120 -bordercolor black -border 2x2 $1 thumb.$1 >/dev/null 2>&1
