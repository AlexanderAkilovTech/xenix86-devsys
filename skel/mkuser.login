# .login - read by the C shell when you log in, after .cshrc.
# From xenix86-devsys: /usr/lib/mkuser/mkuser.login.
setenv SHELL /bin/csh
set ignoreeof			# log out with "logout", not Ctrl-D
set path = ($home/bin /bin /usr/bin .)
if (-x /usr/bin/lastlogin) /usr/bin/lastlogin	# when you last logged in
if (-x /usr/bin/hostname) setenv HOST "`/usr/bin/hostname`"	# this machine's name
# the terminal: the console is an ansi; tset asks about the others
set noglob
set term = (`tset -S -Q -m ansi:ansi -m :\?ansi -r`)
setenv TERM $term[1]
setenv TERMCAP $term[2]
unset term noglob
cursor block >& /dev/null	# a block cursor on the console (it is a line
				# again after the machine is restarted)
source ~/.cshrc			# again, now that TERM is known (the colors)
