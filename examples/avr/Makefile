CC = avr-gcc
CFLAGS = -mmcu=atmega2560 -O1 -g3

SOURCES = ./main.c ./librertos_port.c ../../src/librertos.c
HEADERS = ./librertos_proj.h ./librertos_port.h ../../include/librertos.h
OUTPUTS = main.elf main.hex

main.hex: main.elf
	avr-objcopy -O ihex main.elf main.hex

main.elf: $(SOURCES) $(HEADERS)
	$(CC) -o $@ $(CFLAGS) $(SOURCES) -I. -I../../include
	avr-size --mcu=atmega2560 $@

flash: main.hex
	avrdude -p atmega2560 -c stk500v2 -P /dev/ttyACM0 -b 115200 -U flash:w:main.hex -D

clean:
	rm -f $(OUTPUTS)
