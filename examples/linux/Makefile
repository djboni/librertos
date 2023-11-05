SOURCES = ./main.c ./librertos_port.c ../../src/librertos.c
HEADERS = ./librertos_proj.h ./librertos_port.h ../../include/librertos.h
OUTPUTS = main

main: $(SOURCES) $(HEADERS)
	$(CC) -o $@ $(CFLAGS) $(SOURCES) -I. -I../../include

clean:
	rm -f $(OUTPUTS)
