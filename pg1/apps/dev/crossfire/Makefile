CFLAGS=-I../common -I.. -g

pgcfclient: pg1.o crosshack.o
	gcc -o $@ $^ -lpgui -lpng ../common/libcfclient.a

test: pgcfclient
	./pgcfclient 2>log

fbtest: pgcfclient
	pgserver --appmgr=panel --font_engine=bdf -v fbdev -i rawttykb \
	--themedir=/usr/local/share/picogui/themes -t lcdmono.th \
	-s ./pgcfclient 2>log
