libpgtest: libpgtest.o gl_lucid.th lesson37.o
	g++ -o libpgtest libpgtest.o lesson37.o -lpgserver

libpgtest.o: libpgtest.c
	gcc -c libpgtest.c

lesson37.o: lesson37.cpp
	g++ -c lesson37.cpp

gl_lucid.th: gl_lucid.ths
	themec gl_lucid.ths
