include ../Makefile.rules

BINNAME = mainMenu
SRC     = mainMenu.c calendar.c todo.c
OBJ     = $(patsubst %.c,%.o,$(SRC))
CFLAGS	= -DDEBUG -g -I.
#LDFLAGS += -lefence

$(BINNAME): $(SRC) $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(OBJ)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

clean:
	rm -f $(BINNAME) *.o

install: $(BINNAME)
	cp $(BINNAME) $(PREFIX)/bin

install-strip:
	$(STRIP) $(BINNAME)
	cp $(BINNAME) $(PREFIX)/bin
