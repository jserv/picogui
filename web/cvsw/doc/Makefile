all: permissions ps/themehierarchy.ps ps/serverincludes.ps

clean:
	rm -f `ls ps/* html/* | grep -v CVS`

permissions:
	chmod a+x src/*.pl src/*.sh

ps/themehierarchy.ps: ../pgserver/theme/memtheme.c src/mk_theme_hierarchy.pl
	src/mk_theme_hierarchy.pl

ps/serverincludes.ps: ../pgserver/include/pgserver/*.h
	(cd ../pgserver;cinclude2dot.pl) | src/dot.sh neato serverincludes
