INCLUDE=-I.
LINK=-L.

CC=gcc
CFLAGS=-c -g -Wall -fPIC
LFLAGS=-levent_core -luv -lrt -lm -lpcap

OBJ=listener_tcp.o
OBJ_UDP=recive_udp.o
TARGET=listener_tcp
TARGET_UDP=recive_udp

.PHONY: all clean

%.o: %.c
	$(CC) $(INCLUDE) $(CFLAGS) $< -o $@

all: $(OBJ) $(OBJ_UDP)
	$(CC) $(LINK) $(OBJ) $(LFLAGS) -o $(TARGET)
	$(CC) $(LINK) $(OBJ_UDP) $(LFLAGS) -o $(TARGET_UDP)

clean:
	rm -f $(OBJ) $(OBJ_UDP) $(TARGET) $(TARGET_UDP)
