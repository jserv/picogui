CFLAGS = -g -O2 -lpgui
GCC = gcc

all : sdlremote gpmremote serialremote svgaremote

serialremote : serialremote.c
	$(GCC) -o serialremote serialremote.c $(CFLAGS)

gpmremote : gpmremote.c
	$(GCC) -o gpmremote gpmremote.c $(CFLAGS) -lgpm

svgaremote : svgaremote.c
	$(GCC) -o svgaremote svgaremote.c $(CFLAGS) -lvga -lvgagl

sdlremote : sdlremote.c
	$(GCC) -o sdlremote sdlremote.c $(CFLAGS) $(shell sdl-config --cflags --libs)

clean:
	rm -f sdlremote gpmremote serialremote *.o

