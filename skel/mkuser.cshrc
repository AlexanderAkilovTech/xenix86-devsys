# .cshrc - read by every C shell you start.
# From xenix86-devsys: /usr/lib/mkuser/mkuser.cshrc, which mkuser copies
# into a new account.
set noclobber			# > does not write over a file that is there
set history = 40		# remember the last 40 commands
if ($?prompt) then
	# the prompt: you@host and the directory you are in, in color on the
	# console (TERM ansi); cd, pushd and popd keep it up to date, through
	# /usr/lib/cdprompt.csh
	if (! $?me) set me = `logname`
	if (! $?host) set host = `uname -n`
	if ("$host" == "" || "$host" == "(empty)") set host = xenix
	if (! $?cwd) set cwd = `pwd`	# this csh does not keep $cwd: cd below does
	set color = 0
	if ($?TERM) then
		if ("$TERM" == "ansi") set color = 1
	endif
	if ($color) then
		alias setprompt 'set prompt = "[1m[36m${me}[0m[37m@[1m[32m${host}[0m[37m [1m[33m${cwd}[0m[37m% "'
	else
		alias setprompt 'set prompt = "${me}@${host} ${cwd}% "'
	endif
	alias cd 'set cdarg = (\!*); source /usr/lib/cdprompt.csh'
	if (! $?dirs) set dirs = ()
	alias pushd 'set dirs = ($cwd $dirs); cd \!*'
	alias popd 'cd $dirs[1]; shift dirs'
	setprompt
endif
