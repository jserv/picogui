#!/bin/sh
# Small script to make your initial grep.
PROJECT="pgui"
EMAIL="micahjd@users.sourceforge.net"

for i in `find /usr/local/log/nirvana/2000/ -name access_log`; do
  grep -i $PROJECT.sourceforge.net $i >> /home/groups/$PROJECT/log/access_log;
done

/usr/local/bin/webalizer -i -c /home/groups/$PROJECT/scripts/webalizer.conf > webalize.msg

echo "*********************************************************************">mail.txt
echo "*Initialization grep and webalizer run on pgui.sourceforge.net      *">>mail.txt
echo "*********************************************************************">>mail.txt
echo " ">>mail.txt
echo -n "Run on: ">>mail.txt
'date' >> mail.txt
echo " ">>mail.txt
echo "following output:">>mail.txt
cat webalize.msg >> mail.txt
cat mail.txt | mail -s"[sfcron] loggrepall" $EMAIL
