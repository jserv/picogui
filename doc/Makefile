upload: all
	scp ps/*.ps html/*.jpeg micahjd@shell.sf.net:~/pgui/htdocs/doc/diagram/

all: permissions ps/themehierarchy.ps

clean:
	rm -f `ls ps/* html/* | grep -v CVS`

permissions:
	chmod a+x src/*.pl src/*.sh

ps/themehierarchy.ps: ../pgserver/theme/memtheme.c src/mk_theme_hierarchy.pl
	src/mk_theme_hierarchy.pl

