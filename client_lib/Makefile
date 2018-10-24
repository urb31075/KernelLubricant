CXX = g++
CXXFLAGS += -c -o0 -g -Wall -std=c++17 -I. -I..

EXECUTABLE=client
LIB=ftransfer
LIBRARY=lib$(LIB).a

LDFLAGS += -L.
LDLIBS = -l$(LIB) -lrt
#-Wl,-rpath,.
# -lpthread
# -pthread
SOURCES = client.cpp
LSOURCES = ftransfer.cpp

OBJECTS=$(SOURCES:.cpp=.o)
LOBJECTS=$(LSOURCES:.cpp=.o)

all: $(SOURCES)  $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(LIBRARY)
	$(CXX) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(LIBRARY): $(LOBJECTS)
	ar cr $(LIBRARY) $(LOBJECTS)

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f *.o $(LIBRARY) $(EXECUTABLE)
