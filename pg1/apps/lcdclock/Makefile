include ../Makefile.rules

BINNAME = lcdclock lcdnet
LDFLAGS +=

all: $(BINNAME)

lcdclock: lcdclock.c lcdclock.o
	$(CC) -o $@ $(LDFLAGS) lcdclock.o

lcdnet: lcdnet.c lcdnet.o
	$(CC) -o $@ $(LDFLAGS) lcdnet.o

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

clean:
	rm -f $(BINNAME) *.o

install:
	cp $(BINNAME) $(PREFIX)/bin

install-strip:
	$(STRIP) $(BINNAME)
	cp $(BINNAME) $(PREFIX)/bin
