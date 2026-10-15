# run.sh (M69): a look, no changes, for sudo, crypt and shutdown. What it
# reports of XENIX's own files is only their kind, size and mode, never
# their contents; of /etc/passwd, only each password field's length, and
# the whole field of the throwaway account crypttst alone.
echo "M69 1/3: the shells' paths, and su, haltsys and shutdown"
echo "+++ paths" > report
echo "sh PATH: $PATH" >> report
grep '^root:' /etc/passwd | awk -F: '{print "root home " $6 ", shell [" $7 "]"}' >> report
for f in /bin/su /etc/haltsys /etc/shutdown /bin/login /bin/passwd /etc/group /etc/shadow /etc/sudoers /usr/bin/sudo
do
	if test -f $f
	then
		k=binary
		c=`dd if=$f bs=1 count=1 2>/dev/null`
		test "$c" = "#" -o "$c" = ":" && k=script
		ls -l $f | awk '{print $1, $3, $4, $5}' | sed "s|^|$f $k |" >> report
	else
		echo "$f none" >> report
	fi
done
echo "M69 2/3: the password fields' lengths"
echo "+++ passwd" >> report
awk -F: '{print "len " length($2) ", fields " NF}' /etc/passwd | sort | uniq -c >> report
echo "M69 3/3: crypttst's password field"
echo "+++ crypttst" >> report
grep '^crypttst:' /etc/passwd | awk -F: '{print "hash [" $2 "]"}' >> report
echo "M69: done"
