CFLAGS = -g -O2 -lpgui
CC = gcc

targets : sdlremote infilter_uppercase infilter_circle

serialremote : serialremote.c
	$(CC) -o serialremote serialremote.c $(CFLAGS)

gpmremote : gpmremote.c
	$(CC) -o gpmremote gpmremote.c $(CFLAGS) -lgpm

svgaremote : svgaremote.c
	$(CC) -o svgaremote svgaremote.c $(CFLAGS) -lvga -lvgagl

sdlremote : sdlremote.c
	$(CC) -o sdlremote sdlremote.c $(CFLAGS) $(shell sdl-config --cflags --libs)

infilter_uppercase: infilter_uppercase.c
	$(CC) -o infilter_uppercase infilter_uppercase.c $(CFLAGS)

infilter_circle: infilter_circle.c
	$(CC) -o infilter_circle infilter_circle.c $(CFLAGS) -lm

clean:
	rm -f sdlremote gpmremote serialremote svgaremote infilter_uppercase infilter_circle *.o

