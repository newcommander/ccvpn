INCLUDE=-I.
LINK=-L.

CC=gcc
CXX=g++
CFLAGS=-c -g -Wall -fPIC
LFLAGS=-levent_core -luv -lrt -lm

OBJ=listener.o
TARGET=listener

.PHONY: all clean

%.o: %.cpp
	$(CXX) $(INCLUDE) $(CFLAGS) $< -o $@

all: $(OBJ)
	$(CXX) $(LINK) $(OBJ) $(LFLAGS) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
