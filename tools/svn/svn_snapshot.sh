#!/bin/bash
#
# Usage: svn_snapshot.sh URL Prefix Destination
#
# Export each directory under the given URL into a separate .tar.bz2 file,
# and dump those into the given destination. The given prefix is prepended
# to the package name when making a name for the .tar.bz2 file.
#  --Micah
#

URL=$1
PREFIX=$2
DEST=$3

TMPDIR=/tmp/svn_snapshot.$$
mkdir $TMPDIR

for package in `/usr/bin/env svn list $URL | sed 's/\///'`; do
     /usr/bin/env svn export $URL/$package $TMPDIR/$package
     (cd $TMPDIR; tar jcvf $package.tar.bz2 $package)
     mv $TMPDIR/$package.tar.bz2 $DEST/$PREFIX$package.tar.bz2
done

rm -Rf $TMPDIR
