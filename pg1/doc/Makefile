phony:
	@echo "Specify a target:"
	@echo "    themehierarchy"
	@echo "    doxygen"
	@echo "    pgui-protocol"

themehierarchy: images/themehierarchy.ps 

doxygen: doxygen/client/c/index.html

pgui-protocol: pgui-protocol/pgui-protocol.html

clean:
	rm -f images/themehierarchy* doxygen/client/c/* pgui-protocol/pgui-protocol.html

doxygen/client/c/index.html:
	doxygen src/cli_c_Doxyfile

images/themehierarchy.ps: ../server/theme/memtheme.c src/mk_theme_hierarchy.pl
	src/mk_theme_hierarchy.pl
	
pgui-protocol/pgui-protocol.html: pgui-protocol/pgui-protocol.sgml
	cd pgui-protocol; make onehtml
