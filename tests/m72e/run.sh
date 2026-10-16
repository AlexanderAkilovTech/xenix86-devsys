# run.sh (M72e), run by t as root after the set is installed with custom:
# xtfetch with its colors fixed. Every file the set installed (crc), then xtfetch
# in black and white into the report; look at it in color yourself.
echo "M72e 1/2: the installed files, a dot each (about a minute)"
echo "+++ the lists custom left" > report
ls -l /etc/soft.perms /etc/perms/soft >> report 2>&1
echo "+++ the installed files" >> report
./crc < files.list >> report
echo "M72e 2/2: xtfetch -m"
echo "+++ xtfetch" >> report
ls -l /usr/bin/xtfetch /usr/bin/xtop >> report 2>&1
/usr/bin/xtfetch -m >> report 2>&1
echo "exit $?" >> report
echo "M72e: done. Now try xtfetch yourself, as root and as alex."
