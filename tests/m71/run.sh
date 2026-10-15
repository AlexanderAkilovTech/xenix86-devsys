# run.sh (M71), run by t as root after the set is installed with custom:
# the set for v0.11.0-alpha. Every file the set installed (crc); $USER and
# $LOGNAME in this login and in the new templates; sudo and shutdown in
# place. Each step says on the screen what it is doing.
echo "M71 1/2: the installed files, a dot each (about a minute)"
echo "+++ the lists custom left" > report
ls -l /etc/soft.perms /etc/perms/soft >> report 2>&1
echo "+++ the installed files" >> report
./crc < files.list >> report
echo "M71 2/2: \$USER and \$LOGNAME"
echo "+++ USER" >> report
echo "this login: USER [$USER] LOGNAME [$LOGNAME] logname [`logname`]" >> report
grep -c logname /usr/lib/mkuser/mkuser.login /usr/lib/mkuser/mkuser.prof >> report 2>&1
grep logname /etc/profile /etc/cshrc >> report 2>&1
echo "M71: done"
