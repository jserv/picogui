CFLAGS=-I../common -I.. -g

pgcfclient: pg1.o
	gcc -o $@ $^ -lpgui ../common/libcfclient.a

test: pgcfclient
	./pgcfclient 2>log
