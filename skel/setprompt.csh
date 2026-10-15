# setprompt.csh - the C shell's prompt, for the .cshrc from xenix86-devsys:
# you@host and the directory you are in, your home directory shown as ~,
# in color on the console unless promptcolor is no. The setprompt alias
# sources this, and cd, pushd and popd run it through cdprompt.csh. Only
# the shell's own commands: no process is started for it. (XENIX's csh
# matches a variable in a case label, but has no way to cut the front off
# a name, so the part under ~ is put together from the end, a name at a
# time.)
set pdir = $cwd
if ("$home" != "/") then
	switch ("$cwd")
	case "$home":
		set pdir = "~"
		breaksw
	case "$home"/*:
		set pdtmp = $cwd
		set pdir = ""
		while ("$pdtmp" != "$home" && "$pdtmp" != "/" && "$pdtmp" != "")
			set pdir = /$pdtmp:t$pdir
			set pdtmp = $pdtmp:h
		end
		set pdir = "~$pdir"
		unset pdtmp
		breaksw
	endsw
endif
if ($color) then
	set prompt = "[1m[36m${me}[0m[37m@[1m[32m${host}[0m[37m [1m[33m${pdir}[0m[37m % "
else
	set prompt = "${me}@${host} ${pdir} % "
endif
