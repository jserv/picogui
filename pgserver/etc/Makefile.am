sysconf_DATA = pgserver.conf

EXTRA_DIST = $(sysconf_DATA)

noinst_PROGRAMS = gen-pgserver-conf
gen_pgserver_conf_SOURCES = gen-pgserver-conf.c

pgserver.conf: gen-pgserver-conf $(srcdir)/pgserver.conf.in
	cat $(srcdir)/pgserver.conf.in \
	  | sed "s/@INPUTDRIVERS@/`./gen-pgserver-conf -i`/g" \
	  | sed "s/#.*//g" \
	> pgserver.conf

gen-pgserver-conf:
	$(CC_FOR_BUILD) $< -o $@ -I../include -I$(srcdir)/../gcore

clean:
	rm $(sysconf_DATA)
