CC = gcc
CFLAGS = -O0 -g3

SOURCES = ./librertos_port.c ../../src/librertos.c
HEADERS = ./librertos_proj.h ./librertos_port.h ../../include/librertos.h
OUTPUTS = main

main: ./main.c $(SOURCES) $(HEADERS)
	$(CC) -o $@ $(CFLAGS) $< $(SOURCES) -I. -I../../include

clean:
	rm -f $(OUTPUTS)
