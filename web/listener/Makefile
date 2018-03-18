INCLUDE=-I.
LINK=-L.

CC=/usr/local/gcc-4.8.2/bin/gcc
CXX=/usr/local/gcc-4.8.2/bin/g++
CFLAGS=-c -g -Wall -fPIC
CXXFLAGS=-c -g -Wall -fPIC -std=c++11
LFLAGS=-levent_core -luv -lrt -lm -ljsoncpp

OBJ=listener.o
TARGET=listener

.PHONY: all clean

%.o: %.cpp
	$(CXX) $(INCLUDE) $(CXXFLAGS) $< -o $@

all: $(OBJ)
	$(CXX) $(LINK) $(OBJ) $(LFLAGS) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
