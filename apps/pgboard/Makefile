SRC = pgboard.c
BINNAME = pgboard
EXAMPLE = examples/us_qwerty

all: $(BINNAME) $(EXAMPLE).kb

$(BINNAME): $(SRC)
	gcc -o $(BINNAME) $(SRC) -lpgui -g

$(EXAMPLE).kb: $(EXAMPLE).kbs
	perl kbcompile.pl < $(EXAMPLE).kbs > $(EXAMPLE).kb

clean:
	rm -f $(BINNAME) *.o examples/*.kb

