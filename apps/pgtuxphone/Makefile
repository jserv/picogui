# Set this to cross compile
CROSS	= 
#CROSS	= arm-linux
#CROSS	= arm-uclibc-

# Change this to the path of your cli_c headers
INCLUDE = 

# Change this to your install prefix
PREFIX	= /usr

# Programs we need
CC      = $(CROSS)gcc
LD	= $(CROSS)ld
AR	= $(CROSS)ar
STRIP   = $(CROSS)strip

LDFLAGS	= -lpgui
CFLAGS	= -O3 -fno-strict-aliasing -g

BINNAME = pgtuxphone 
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
