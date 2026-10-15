# .cshrc - read by every C shell you start.
# From xenix86-devsys: /usr/lib/mkuser/mkuser.cshrc, which mkuser copies
# into a new account.
set noclobber			# > does not write over a file that is there
set history = 10		# remember the last 10 commands
#set promptcolor = no		# the prompt without its colors
if ($?prompt) then
	# the prompt: you@host and the directory you are in, ~ for your home
	# directory, in color on the console (TERM ansi); cd, pushd and popd
	# keep it up to date, through /usr/lib/cdprompt.csh and setprompt.csh
	if (! $?me) set me = `logname`
	if (! $?host) then
		if ($?HOST) then
			set host = "$HOST"
		else
			set host = `hostname`
		endif
	endif
	if ("$host" == "") set host = xenix
	if (! $?cwd) set cwd = `pwd`	# this csh does not keep $cwd: cd below does
	set color = 0
	if ($?TERM) then
		if ("$TERM" == "ansi") set color = 1
	endif
	if ($?promptcolor) then
		if ("$promptcolor" == "no") set color = 0
	endif
	alias setprompt 'source /usr/lib/setprompt.csh'
	alias cd 'set cdarg = (\!*); source /usr/lib/cdprompt.csh'
	if (! $?dirs) set dirs = ()
	alias pushd 'set dirs = ($cwd $dirs); cd \!*'
	alias popd 'cd $dirs[1]; shift dirs'
	setprompt
endif
