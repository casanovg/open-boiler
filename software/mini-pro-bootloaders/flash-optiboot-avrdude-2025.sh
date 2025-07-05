#!/bin/sh

#  ==================================================================================
# || NOTE: This script runs from Unix-like shells like Git Bash or VS Code terminal ||
#  ==================================================================================

C:/Users/casanovg/Downloads/USBasp/avrdude-v8.0-windows-x64/avrdude.exe -v -c USBasp -p ATmega328P -P usb -U flash:w:optiboot_atmega328-mini.hex:i -U lfuse:w:0xFF:m -U hfuse:w:0xDA:m -U efuse:w:0x05:m

#avrdude -c USBasp -p ATmega328P -U flash:w:optiboot_flash_atmega328_UART0_115200_16000000L.hex:i -U lfuse:w:0xFF:m -U hfuse:w:0xDA:m -U efuse:w:0x05:m
