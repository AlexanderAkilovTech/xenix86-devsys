# run.sh (M66b, M66 again after a reinstall), run by t after the set is installed with custom: every file
# the set installed (crc, checked on the host); hostname giving the kernel
# the node name, as uname -n shows it; date taking the time as people write
# it, and asktime the same, the clock kept and put back; uptime; and the
# start-up files' $HOST. Each step says on the screen what it is doing.
echo "M66b 1/5: the installed files, a dot each (about a minute)"
echo "+++ the lists custom left" > report
ls -l /etc/soft.perms /etc/perms/soft /tmp/soft.perms.old >> report 2>&1
echo "+++ the installed files" >> report
./crc < files.list >> report
echo "M66b 2/5: hostname and uname -n"
echo "+++ hostname and uname -n" >> report
echo "before: `uname -n`" >> report
hostname >> report 2>&1
hostname `hostname` >> report 2>&1
echo "status $?" >> report
echo "after: `uname -n`" >> report
echo "M66b 3/5: date and asktime, the clock kept and put back"
echo "+++ date and asktime" >> report
ls -l /bin/date /etc/asktime /usr/bin/uptime /etc/default/date >> report 2>&1
./settime save >> report 2>&1
for a in "3:36 PM" "10/14 9:05 AM" "2026-10-14 15:36" "10/14/26 9:05" "1536" "1013120098" "13:00 PM" "2026-14-10"
do
	echo "== date $a" >> report
	date $a >> report 2>&1
	echo "status $?" >> report
done
echo "== asktime, Return" >> report
echo "" | /etc/asktime >> report 2>&1
echo "" >> report
echo "== asktime, 5:36 PM" >> report
echo "5:36 PM" | /etc/asktime >> report 2>&1
echo "== asktime, soon, then 2610141525" >> report
(echo soon; echo 2610141525) | /etc/asktime >> report 2>&1
echo "== asktime -a, no input" >> report
/etc/asktime -a < /dev/null >> report 2>&1
echo "" >> report
./settime back >> report 2>&1
date >> report 2>&1
echo "M66b 4/5: uptime"
echo "+++ uptime" >> report
uptime >> report 2>&1
echo "status $?" >> report
echo "M66b 5/5: \$HOST"
echo "+++ HOST" >> report
csh -f hostcheck.csh >> report 2>&1
echo "M66b: done"
