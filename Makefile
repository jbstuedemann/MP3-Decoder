CXX = g++-10
CXXFLAGS = -Wall -Wl,-stack_size -Wl,400000000 -g -std=c++17

EXECS = main

all: $(EXECS)

main: main.cpp mp3.h mp3.cc huffman.h huffman.cc tables.h
	$(CXX) $(CXXFLAGS) -o main main.cpp mp3.h mp3.cc huffman.h huffman.cc tables.h

test: main
	./main

clean:
	rm -f main
