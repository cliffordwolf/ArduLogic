
ARDUINO_TTY=/dev/ttyACM0

%.elf: %.c
	avr-gcc -Wall -O3 -std=gnu99 -mmcu=atmega328p -DF_CPU=16000000L -o $@ $^

%.hex: %.elf
	avr-objcopy -j .text -j .data -O ihex $^ $@

%.prog: %.hex
	avrdude -p m328p -b 115200 -c arduino -P $(ARDUINO_TTY) -v -U flash:w:$^

