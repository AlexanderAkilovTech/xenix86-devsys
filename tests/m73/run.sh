# run.sh (M73), run by t as root after the set is installed with custom:
# xtop catches kill's default signal, 15, and leaves the screen and the
# terminal as q does. Every file the set installed (crc), then the kill test,
# by hand, on two consoles.
echo "M73 1/1: the installed files, a dot each (about a minute)"
echo "+++ the lists custom left" > report
ls -l /etc/soft.perms /etc/perms/soft >> report 2>&1
echo "+++ the installed files" >> report
./crc < files.list >> report
ls -l /usr/bin/xtop /usr/bin/top /usr/bin/xtfetch >> report 2>&1
echo "M73: done. Now the kill test:"
echo "  console 1, as root: xtop"
echo "  console 2, as alex: ps -ef | grep xtop, then sudo kill <its PID>"
echo "  back on console 1: the screen should be cleared, a clean # prompt,"
echo "  no Terminated, and typing should echo normally."
