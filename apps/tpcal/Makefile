BINNAME = tpcal
SRC     = $(wildcard *.c)
OBJ     = $(patsubst %.c,%.o,$(SRC))
CC      = gcc
LDFLAGS = -lpgui

$(BINNAME): $(SRC) $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(OBJ)

.c.o:
	$(CC) $(CFLAGS) -I../../cli_c/include -c $<

clean:
	rm -f $(BINNAME) *.o

install:
	cp $(BINNAME) /usr/local/bin
