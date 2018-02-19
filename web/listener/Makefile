INCLUDE=-I.
LINK=-L.

CC=gcc
CFLAGS=-c -g -Wall -fPIC
LFLAGS=-levent_core -luv -lrt -lm

OBJ=listener.o
TARGET=listener

.PHONY: all clean

%.o: %.c
	$(CC) $(INCLUDE) $(CFLAGS) $< -o $@

all: $(OBJ)
	$(CC) $(LINK) $(OBJ) $(LFLAGS) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
