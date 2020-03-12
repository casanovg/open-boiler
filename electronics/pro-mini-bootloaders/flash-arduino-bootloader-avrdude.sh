#!/bin/sh
avrdude -v -c USBasp -p ATmega328P -P usb -U flash:w:atmegaboot_168_atmega328.hex:i -U lfuse:w:0xFF:m -U hfuse:w:0xDA:m -U lock:w:0x0F:m

#avrdude -v -c USBasp -p ATmega328P -P usb -U flash:w:ATmegaBOOT_168_atmega328.hex:i -U lfuse:w:0xFF:m -U hfuse:w:0xDA:m -U efuse:w:0x05:m -Ulock:w:0x0F:m

#avrdude -v -p atmega328p -c usbasp -P usb -U flash:w:C:\Users\casanovg\AppData\Local\Arduino15\packages\arduino\hardware\avr\1.6.23/bootloaders/atmega/ATmegaBOOT_168_atmega328.hex:i -Ulock:w:0x0F:m