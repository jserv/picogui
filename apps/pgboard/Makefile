SRC = pgboard.c kbfile.c
BINNAME = pgboard

EXAMPLES = $(wildcard examples/*.kbs)

all: $(BINNAME) $(EXAMPLES:.kbs=.kb)

$(BINNAME): $(SRC)
	gcc -o $(BINNAME) $(SRC) -lpgui -g

%.kb : %.kbs kbcompile.pl
	perl kbcompile.pl < $< > $@

clean:
	rm -f $(BINNAME) *.o examples/*.kb

