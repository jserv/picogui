OBJS = EmbeddedPGserver.o jetengine.o PythonThread.o
HEADERS = EmbeddedPGserver.h PythonThread.h SimpleException.h
BIN = jetengine

# Embedding pgserver and python
# We could use a dynamic library for pgserver, but in a
# real world situation it's better to statically link it
# since this program will require a specific configuration
# of pgserver.
# Python usually isn't distributed in a shared library,
# only static.

# Libs for a static pgserver and static python
#LIBS += -lGL `sdl-config --libs` -ljpeg -lpng -lfreetype \
#	../../../pgserver/.libs/libpgserver.a \
#	-lutil /usr/lib/python2.2/config/libpython2.2.a \
#	-lGLU
#CXXFLAGS += -I/usr/include/python2.2 -g

# Libs for a dynamic pgserver and static python
LIBS += -lpgserver \
	-lutil /usr/lib/python2.2/config/libpython2.2.a
CXXFLAGS += -I/usr/include/python2.2 -g

$(BIN): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIBS) 

%.o: %.cpp $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@

clean:
	rm $(OBJS) $(BIN)