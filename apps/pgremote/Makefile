CFLAGS = -g -O2 -lpgui
GCC = gcc

all : sdlremote gpmremote serialremote


serialremote : serialremote.c
	$(GCC) -o serialremote serialremote.c $(CFLAGS)

gpmremote : gpmremote.c
	$(GCC) -o gpmremote gpmremote.c $(CFLAGS) -lgpm

sdlremote : sdlremote.c
	$(GCC) -o sdlremote sdlremote.c $(CFLAGS) $(shell sdl-config --cflags --libs)

clean:
	rm -f sdlremote gpmremote serialremote *.o

