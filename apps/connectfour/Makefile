connectfour: connectfour.o rules.o ai.o
	gcc -o connectfour connectfour.o rules.o ai.o -lpgui

run: connectfour
	./connectfour

connectfour.o: connectfour.c connectfour.h
	gcc -c connectfour.c 

rules.o: rules.c rules.h
	gcc -c rules.c 

ai.o: ai.c ai.h
	gcc -c ai.c

clean:
	rm -Rf *~ *.o connectfour
