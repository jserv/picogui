#!/bin/sh

for i in *.xwt; do
    echo "compiling $i"
    python2.2 ./xwtcompile.py $i $(basename $i .xwt).wt
done