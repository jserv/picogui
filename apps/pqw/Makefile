LDFLAGS= -lpgui
# CPPFLAGS = -g -Wall
CPPFLAGS = -Os -Wall
OBJS=pqw.o
headers=pics.h

all: pqw

pqw: $(OBJS)
	$(CC) -o pqw $(OBJS) $(CPPFLAGS) $(LDFLAGS)

pics.h: pbm_dump alt.pbm lock.pbm numbers.pbm upper_case.pbm ctl.pbm keys.pbm lower_case.pbm  symbols.pbm
	rm -f pics.h
	./pbm_dump alt.pbm >> pics.h
	./pbm_dump ctl.pbm >> pics.h
	./pbm_dump lock.pbm >> pics.h
	./pbm_dump upper_case.pbm >> pics.h
	./pbm_dump lower_case.pbm >> pics.h
	./pbm_dump numbers.pbm >> pics.h
	./pbm_dump symbols.pbm >> pics.h
	./pbm_dump keys.pbm >> pics.h

clean:
	rm -f *.o pqw pbm_dump *~


