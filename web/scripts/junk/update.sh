#!/bin/sh
cd /home/groups/pgui/cvsw
(cd cli_c;make demos)
(cd doc;make)
(cd pgserver;make)
(cd themetools/themec;make)
