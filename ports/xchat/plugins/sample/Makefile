LDFLAGS = -ggdb
CFLAGS = -Wall
CC = gcc

sample.so:	sample.c
	$(CC) $(LDFLAGS) $(CFLAGS) -I../../ -DSKIPCONFIG -shared -Wl,-soname,sample.so -o $@ $< -lc `gtk-config --cflags`
