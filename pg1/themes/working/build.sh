#!/bin/bash
# quick and dirty script to build all themes

if [ "$#" -ge 1 ]
then
  instdir="$1"
else
  instdir="/usr/local/share/picogui/themes"
fi
mkdir -p "$instdir"
echo "installing in $instdir"

topdir="`pwd`"
for dir in `find $topdir -maxdepth 1 -type d`
do
  cd $dir
  if ! ls *.ths >/dev/null 2>/dev/null
  then
      continue
  fi
  for ths in *.ths
  do
    themec >"$instdir/${ths%s}" <"$ths"
  done
done
