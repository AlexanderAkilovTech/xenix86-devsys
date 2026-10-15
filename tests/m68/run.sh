# run.sh (M68), run by t after the set is installed with custom: every file
# the set installed (crc, checked on the host); the C shell prompt with a
# space before the %, in color and without; and alex's .login given its
# block cursor line, as the new template has it (the old one kept, as
# .login.old). Each step says on the screen what it is doing.
echo "M68 1/3: the installed files, a dot each (about a minute)"
echo "+++ the lists custom left" > report
ls -l /etc/soft.perms /etc/perms/soft >> report 2>&1
echo "+++ the installed files" >> report
./crc < files.list >> report
echo "M68 2/3: the prompt, in color and without"
echo "+++ the prompt" >> report
csh -f prompt.csh >> report 2>&1
echo "M68 3/3: the block cursor in alex's .login"
echo "+++ alex's .login" >> report
h=`grep '^alex:' /etc/passwd | cut -d: -f6`
if test -n "$h" -a -f "$h/.login"
then
	if grep '^#cursor block' $h/.login > /dev/null
	then
		cp $h/.login $h/.login.old
		sed 's/^#cursor block/cursor block/' $h/.login.old > $h/.login
		echo "changed" >> report
	else
		echo "no commented cursor line" >> report
	fi
	grep cursor $h/.login >> report
else
	echo "no alex, or no .login" >> report
fi
echo "M68: done"
