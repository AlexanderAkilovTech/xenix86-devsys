# run.sh (M70), run by t as root after the set is installed with custom:
# every file the set installed (crc); our crypt on XENIX against XENIX's
# own hash of test1234 (M69); sudo as root, and as alex, who may not use
# it until /etc/default/sudo says so; shutdown's messages (it is not let
# go on: the halt itself is tried by hand). Each step says what it does.
echo "M70 1/4: the installed files, a dot each (about a minute)"
echo "+++ the lists custom left" > report
ls -l /etc/soft.perms /etc/perms/soft >> report 2>&1
echo "+++ the installed files" >> report
./crc < files.list >> report
echo "M70 2/4: crypt on XENIX (a few seconds)"
echo "+++ crypt" >> report
echo "j9test1234" | ./cryptt >> report 2>&1
echo "M70 3/4: sudo as root, and as alex"
echo "+++ sudo" >> report
ls -l /usr/bin/sudo /etc/default/sudo /usr/adm/sudolog >> report 2>&1
echo "== root: sudo ./ids" >> report
sudo ./ids >> report 2>&1
echo "status $?" >> report
echo "== alex: sudo ./ids" >> report
su alex -c "sudo `pwd`/ids" >> report 2>&1
echo "status $?" >> report
echo "== the log" >> report
cat /usr/adm/sudolog >> report 2>&1
echo "M70 4/4: shutdown's messages"
echo "+++ shutdown" >> report
ls -l /usr/bin/shutdown >> report 2>&1
shutdown -h soon >> report 2>&1
echo "status $?" >> report
su alex -c "/usr/bin/shutdown -h now" >> report 2>&1
echo "status $?" >> report
echo "M70: done"
