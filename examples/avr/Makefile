CC = avr-gcc
CFLAGS = -mmcu=atmega2560 -O1 -g3
SERIAL = /dev/ttyACM0

SOURCES = ./librertos_port.c ../../src/librertos.c
HEADERS = ./librertos_proj.h ./librertos_port.h ../../include/librertos.h
OUTPUTS = librertos.o main.elf main.hex tests.elf tests.hex

main.elf: ./main.c $(SOURCES) $(HEADERS)
	$(CC) -o $@ $(CFLAGS) $< $(SOURCES) -I. -I../../include
	avr-objcopy -O ihex $@ main.hex
	avr-size --mcu=atmega2560 $@

flash: main.elf
	avrdude -p atmega2560 -c stk500v2 -P $(SERIAL) -b 115200 -U flash:w:main.hex -D -V

tests.elf: ./tests.c $(SOURCES) $(HEADERS)
	$(CC) -o $@ $(CFLAGS) $< $(SOURCES) -I. -I../../include
	avr-objcopy -O ihex $@ tests.hex
	avr-size --mcu=atmega2560 $@

flash_tests: tests.elf
	avrdude -p atmega2560 -c stk500v2 -P $(SERIAL) -b 115200 -U flash:w:tests.hex -D -V

clean:
	rm -f $(OUTPUTS)

CFLAGS_DISABLE = -DLIBRERTOS_DISABLE_TIMERS=1 -DLIBRERTOS_DISABLE_SEMAPHORES=1 -DLIBRERTOS_DISABLE_MUTEXES=1 -DLIBRERTOS_DISABLE_QUEUES=1
CFLAGS_OSIZE = -Os

size:
	@$(CC) -c -o librertos.o $(CFLAGS) -DNDEBUG $(CFLAGS_OSIZE) ../../src/librertos.c -I. -I../../include
	@avr-size --mcu=atmega2560 librertos.o
	@$(CC) -c -o librertos.o $(CFLAGS) -DNDEBUG $(CFLAGS_OSIZE) $(CFLAGS_DISABLE) ../../src/librertos.c -I. -I../../include
	@avr-size --mcu=atmega2560 librertos.o | grep -v text
	@$(CC) -c -o librertos.o $(CFLAGS) -DNDEBUG ../../src/librertos.c -I. -I../../include
	@avr-size --mcu=atmega2560 librertos.o | grep -v text
	@$(CC) -c -o librertos.o $(CFLAGS) -DNDEBUG $(CFLAGS_DISABLE) ../../src/librertos.c -I. -I../../include
	@avr-size --mcu=atmega2560 librertos.o | grep -v text
