#!/bin/sh
# Small shell script to grab logs outta the main log file and put them
# in your groups logs, run webalize and send you a summary email about
# the run.

# Edit these lines to suit your needs
PROJECT="pgui"
EMAIL="micahjd@users.sourceforge.net"

YEAR=`date --date="1 day ago" +%Y`
MONTH=`date --date="1 day ago" +%m`
DAY=`date --date="1 day ago" +%d`

# cd to todays log files
cd /usr/local/log/nirvana/$YEAR/$MONTH/$DAY

grep -i $PROJECT.sourceforge.net access_log >> /home/groups/$PROJECT/log/access_log

cd /home/groups/$PROJECT/scripts
/usr/local/bin/webalizer -c /home/groups/$PROJECT/scripts/webalizer.conf > webalize.msg 2>&1

echo "********************************************************">mail.txt
echo "*Cron job on pgui.sourceforge.net for webalizer.       *">>mail.txt
echo "********************************************************">>mail.txt
echo " ">>mail.txt
echo -n "Run on: ">>mail.txt
'date' >> mail.txt
echo " ">>mail.txt
echo "following output:">>mail.txt
cat webalize.msg >> mail.txt
cat mail.txt | mail -s"[sfcron] webalizer" $EMAIL
