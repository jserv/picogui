all: images/themehierarchy.ps

clean:
	rm -f images/themehierarchy*

images/themehierarchy.ps: ../server/theme/memtheme.c src/mk_theme_hierarchy.pl
	src/mk_theme_hierarchy.pl
	
