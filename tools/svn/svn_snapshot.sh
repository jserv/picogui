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
DEST=$2
TMPDIR=/tmp/svn_snapshot.$$
mkdir $TMPDIR

for package in `svn list $URL | sed 's/\///'`; do
     svn export $URL/$package $TMPDIR/$package
     (cd $TMPDIR; tar jcvf $package.tar.bz2 $package)
     mv $TMPDIR/$package.tar.bz2 $DEST/$prefix$package.tar.bz2
done

rm -Rf $TMPDIR
