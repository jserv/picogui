#!/bin/sh

# Make today's tarball
DATECODE=`date "+%Y%m%d"`
cd /home/groups/pgui/htdocs/cvstgz
(
mkdir pgui-dev$DATECODE
cd pgui-dev$DATECODE
cvs -d:pserver:anonymous@cvs1:/cvsroot/pgui co cli_c
cvs -d:pserver:anonymous@cvs1:/cvsroot/pgui co cli_perl
cvs -d:pserver:anonymous@cvs1:/cvsroot/pgui co doc
#cvs -d:pserver:anonymous@cvs1:/cvsroot/pgui co images
cvs -d:pserver:anonymous@cvs1:/cvsroot/pgui co pgserver
cvs -d:pserver:anonymous@cvs1:/cvsroot/pgui co themetools
cd ..
tar zcvf pgui-dev$DATECODE.tar.gz pgui-dev$DATECODE
rm -R pgui-dev$DATECODE

# Clean up files more than a month old
find -ctime +30 | xargs rm -f

rm pgui-dev-latest.tar.gz
ln -s pgui-dev$DATECODE.tar.gz pgui-dev-latest.tar.gz
) > snapshot.log.txt 2>&1
