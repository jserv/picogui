#!/bin/bash
#
# Usage: svn_snapshot_local.sh WorkingCopy Prefix Destination
#
# Export each directory under the given URL into a separate .tar.bz2 file,
# and dump those into the given destination. The given prefix is prepended
# to the package name when making a name for the .tar.bz2 file.
#  --Micah
#

SVN=/usr/local/bin/svn
WC=$1
PREFIX=$2
DEST=$3

TMPDIR=/tmp/svn_snapshot.$$
mkdir $TMPDIR

(cd $WC; svn up)
for package in `(cd $WC; ls)`; do
     $SVN export $WC/$package $TMPDIR/$package
     (cd $TMPDIR; tar jcvf $package.tar.bz2 $package)
     mv $TMPDIR/$package.tar.bz2 $DEST/$PREFIX$package.tar.bz2
     chmod a+r $DEST/$PREFIX$package.tar.bz2
done

rm -Rf $TMPDIR
