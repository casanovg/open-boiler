#!bin/sh
avrdude -c usbasp -p atmega328p -U flash:w:victoria-control.hex:i
#-U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
