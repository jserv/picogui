include ../Makefile.rules

BINNAME = omnibar-helio
SRC     = $(wildcard *.c)
OBJ     = $(patsubst %.c,%.o,$(SRC))
LDFLAGS +=

$(BINNAME): $(SRC) $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(OBJ)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

clean:
	rm -f $(BINNAME) *.o

install:
	cp $(BINNAME) $(PREFIX)/bin

install-strip:
	$(STRIP) $(BINNAME)
	cp $(BINNAME) $(PREFIX)/bin
