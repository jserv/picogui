SRC = sdlremote.c
BINNAME = pgremote

$(BINNAME) : $(SRC)
	gcc -o $(BINNAME) $(SRC) -lpgui -g $(shell sdl-config --cflags --libs)

clean:
	rm -f $(BINNAME) *.o

