OBJS = EmbeddedPGserver.o jetengine.o
HEADERS = EmbeddedPGserver.h
BIN = jetengine
LIBS = -lpgserver

$(BIN): $(OBJS)
	$(CXX) -o $@ $(LIBS) $(OBJS)

%.o: %.cpp $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@

