SRC = main.c ptyfork.c

pterm: $(SRC)
	gcc -o pterm $(SRC) -lpgui -g

install:
	install pterm /usr/local/bin/pterm

clean:
	rm -f pterm *.o

