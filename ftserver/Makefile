CXX = g++
CXXFLAGS += -c -o0 -g -Wall -std=c++17 -I. -I..

LDFLAGS += -lrt
# -lpthread
# -pthread
SOURCES = server.cpp
DEPS = ../common/cwrdefs.h

OBJECTS=$(SOURCES:.cpp=.o)

EXECUTABLE=server

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@  $(LDFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f *.o $(EXECUTABLE)
