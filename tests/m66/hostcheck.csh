# hostcheck.csh (M66): the HOST line of the new .login, and the .cshrc's
# prompt taking the machine's name from $HOST when it is set
if (-x /usr/bin/hostname) setenv HOST "`/usr/bin/hostname`"
echo "HOST from the .login line: $HOST"
set prompt = x
setenv TERM dumb
setenv HOST fromhost
source /usr/lib/mkuser/mkuser.cshrc
echo "the prompt's host with HOST set: $host"
