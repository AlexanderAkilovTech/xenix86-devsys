# run.sh (M67), run by t after the set is installed with custom: every file
# the set installed (crc, checked on the host); xtop's uptime from the
# clock's ticks, before and after the clock is set years ahead, beside
# uptime's; and the new .cshrc's history of 10. Each step says on the
# screen what it is doing.
echo "M67 1/3: the installed files, a dot each (about a minute)"
echo "+++ the lists custom left" > report
ls -l /etc/soft.perms /etc/perms/soft >> report 2>&1
echo "+++ the installed files" >> report
./crc < files.list >> report
echo "M67 2/3: xtop's uptime, the clock set ahead and put back (about a minute)"
echo "+++ xtop and uptime" >> report
./settime save >> report 2>&1
echo "== before" >> report
uptime >> report 2>&1
TERM=ansi xtop -b -n 1 -d 1 2>&1 | grep Uptime >> report
date 1/1/2030 12:00 PM >> report 2>&1
echo "== the clock set to 2030" >> report
uptime >> report 2>&1
TERM=ansi xtop -b -n 1 -d 1 2>&1 | grep Uptime >> report
./settime back >> report 2>&1
date >> report 2>&1
echo "M67 3/3: the new .cshrc's history"
echo "+++ history" >> report
grep history /usr/lib/mkuser/mkuser.cshrc >> report 2>&1
echo "M67: done"
