CC=gcc
CFLAGS=-g -Wall
LIBS=-lefence -lpgui

all: picomail

clean:
	rm -f *.o
	rm -f .*.swp
	rm -f *~

picomail: tcpclient.o imap.o picomail.o configfile.o
	$(CC) $(CFLAGS) $(LIBS) \
					-o	picomail \
						picomail.o \
						tcpclient.o \
						imap.o \
						configfile.o
						

picomail.o: picomail.c 
	$(CC) $(CFLAGS) -c -o 	picomail.o \
							picomail.c

tcpclient.o: tcpclient.c tcpclient.h
	$(CC) $(CFLAGS) -c -o 	tcpclient.o \
							tcpclient.c

imap.o: imap.c imap.h
	$(CC) $(CFLAGS) -c -o 	imap.o \
							imap.c

configfile.o: configfile.c configfile.h
	$(CC) $(CFLAGS) -c -o	configfile.o \
							configfile.c
