# prompt.csh (M68): the prompt that setprompt.csh makes, in color and not
set me = alex host = xenix86 home = /usr/alex cwd = /usr/alex
set color = 1
source /usr/lib/setprompt.csh
echo "color: [$prompt]"
set color = 0
source /usr/lib/setprompt.csh
echo "plain: [$prompt]"
