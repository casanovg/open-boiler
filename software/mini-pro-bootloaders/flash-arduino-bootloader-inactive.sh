#!/bin/sh
avrdude -v -c USBasp -p ATmega328P -P usb -U flash:w:atmegaboot_168_atmega328.hex:i -U lfuse:w:0xFF:m -U hfuse:w:0xDF:m -U lock:w:0xFF:m

