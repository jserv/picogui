all: images/themehierarchy.ps doxygen/client/c/index.html

clean:
	rm -f images/themehierarchy* doxygen/client/c/*

doxygen/client/c/index.html:
	doxygen src/cli_c_Doxyfile

images/themehierarchy.ps: ../server/theme/memtheme.c src/mk_theme_hierarchy.pl
	src/mk_theme_hierarchy.pl
	
