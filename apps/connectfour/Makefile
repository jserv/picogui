connectfour: connectfour.o rules.o ai.o ai1.o ai2.o ai3.o ai4.o ai5.o ai6.o
	gcc -o connectfour *.o -lpgui

run: connectfour
	./connectfour

connectfour.o: connectfour.c connectfour.h
	gcc -c connectfour.c 

rules.o: rules.c rules.h
	gcc -c rules.c 

ai.o: ai.c ai.h
	gcc -c ai.c

ai1.o: ai1.c ai1.h
	gcc -c ai1.c

ai2.o: ai2.c ai2.h
	gcc -c ai2.c

ai3.o: ai3.c ai3.h
	gcc -c ai3.c

ai4.o: ai4.c ai4.h
	gcc -c ai4.c

ai5.o: ai5.c ai5.h
	gcc -c ai5.c

ai6.o: ai6.c ai6.h
	gcc -c ai6.c

clean:
	rm -Rf *~ *.o connectfour
