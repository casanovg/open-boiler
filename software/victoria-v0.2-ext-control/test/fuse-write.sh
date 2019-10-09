#!/bin/sh
#echo ""
#echo "***********************************************************************"
#echo "*                                                                     *"
#echo "* Please use USBasp for flashing Timonel to the ATtiny device         *"
#echo "* =================================================================== *"
#echo "* 2019-08-09 Gustavo Casanova                                         *"
#echo "***********************************************************************"
#echo ""

avrdude -c USBasp -p m328p -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m;

