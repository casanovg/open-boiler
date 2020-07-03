#!/bin/sh
echo ""
echo "****************************************"
echo "*                                      *"
echo "*  AVR microcontroller fuses settings  *"
echo "*  ..................................  *"
echo "*  2018-10-20 Gustavo Casanova         *"
echo "*                                      *"
echo "****************************************"
echo ""
ARG1=${1:-fuse-settings.txt}
LF="0x62"
HF="0xd5"
EF="0xfe"
if avrdude -v -c USBasp -p ATmega328P -P usb -U lfuse:r:lfuse.hex:h -U hfuse:r:hfuse.hex:h -U efuse:r:efuse.hex:h 2>>/dev/null;

then
	echo "AVR microcontroller fuses" > $ARG1;
	echo "=========================" >> $ARG1;
    echo "Low fuse = [`cat lfuse.hex`]" | tee -a $ARG1;
    echo ".............................." | tee -a $ARG1;
    echo "High fuse = [`cat hfuse.hex`]" | tee -a $ARG1;
    echo ".............................." | tee -a $ARG1;
    echo "Extended fuse = [`cat efuse.hex`]" | tee -a $ARG1;
    echo ".............................." | tee -a $ARG1;
	rm lfuse.hex hfuse.hex efuse.hex;
else
	echo "AVRdude execution error!";
fi
	