connectfour: connectfour.o rules.o ai.o ai1.o ai2.o ai3.o #ai4.o
	gcc -o connectfour *.o -lpgui

run: connectfour
	./connectfour

connectfour.o: connectfour.c connectfour.h
	gcc -c connectfour.c 

rules.o: rules.c rules.h
	gcc -c rules.c 

ai.o: ai.c ai.h
	gcc -c ai.c

ai1.o: ai1.c
	gcc -c ai1.c

ai2.o: ai2.c 
	gcc -c ai2.c

ai3.o: ai3.c 
	gcc -c ai3.c

ai4.o: ai4.c 
	gcc -c ai4.c


clean:
	rm -Rf *~ *.o connectfour
