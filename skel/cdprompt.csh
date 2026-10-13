# cdprompt.csh - the C shell's cd, for the .cshrc from xenix86-devsys:
# change directory, keep $cwd (which XENIX's csh does not keep itself) and
# redraw the prompt. The cd alias sets cdarg to cd's argument and sources
# this file, so that the usual cases start no process: no name, .., a name
# in this directory, or a whole path; anything else asks pwd. If chdir
# fails, csh stops reading here, and $cwd and the prompt stay as they were.
# The cases are told apart with switch, whose patterns XENIX's csh has;
# it has no pattern operators in if.
if ($#cdarg == 0) then
	chdir
	set cwd = $home
else
	chdir $cdarg[1]
	switch ("$cdarg[1]")
	case ..:
		set cwd = $cwd:h
		if ("$cwd" == "") set cwd = /
		breaksw
	case *.*:
	case ?*/:
		set cwd = `pwd`
		breaksw
	case /*:
		set cwd = "$cdarg[1]"
		breaksw
	case */*:
		set cwd = `pwd`
		breaksw
	default:
		if ("$cwd" == "/") then
			set cwd = /$cdarg[1]
		else
			set cwd = $cwd/$cdarg[1]
		endif
		breaksw
	endsw
endif
setprompt
