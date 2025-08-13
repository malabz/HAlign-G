CXX = g++
CXXFLAGS = -O3 -std=c++17 -pthread

SRCDIR = .
SOURCES = $(wildcard $(SRCDIR)/**/*.cpp $(SRCDIR)/**/*.cc)
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = ../mynuc

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(SRCDIR)/**/*.o $(EXECUTABLE)
