#!/bin/sh

DATECODE=`date "+%Y%m%d"`

cvs -dmicahjd@cvs1:/cvsroot/pgui import -m "Import from web site" web vendor web$DATECODE  
