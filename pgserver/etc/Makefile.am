sysconf_DATA = pgserver.conf

EXTRA_DIST = $(sysconf_DATA) pgserver.conf.in

noinst_PROGRAMS = gen-pgserver-conf
gen_pgserver_conf_SOURCES = gen-pgserver-conf.c

pgserver.conf: configurator
	$(SHELL) configurator > pgserver.conf

configurator: gen-pgserver-conf configurator.mk
	cat configurator.mk \
	  | sed "s/@INPUTDRIVERS@/`./gen-pgserver-conf -i`/g" \
	> configurator

gen-pgserver-conf: gen-pgserver-conf.c ../include/pgserver/autoconf.h
	$(CC_FOR_BUILD) $< -o $@ -I../include -I$(srcdir)/../gcore
