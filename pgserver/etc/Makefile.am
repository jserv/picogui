sysconf_DATA = pgserver.conf

EXTRA_DIST = $(sysconf_DATA)

pgserver.conf: gen-pgserver-conf $(srcdir)/pgserver.conf.in
	cat $(srcdir)/pgserver.conf.in \
	  | sed "s/@INPUTDRIVERS@/`./gen-pgserver-conf -i`/g" \
	  | sed "s/#.*//g" \
	> pgserver.conf

gen-pgserver-conf:	$(srcdir)/gen-pgserver-conf.c \
			../include/pgserver/autoconf.h \
			$(srcdir)/../gcore/inputdrivers.inc
	gcc -I../include -I$(srcdir)/../gcore $(srcdir)/gen-pgserver-conf.c -o gen-pgserver-conf

clean:
	rm pgserver.conf
