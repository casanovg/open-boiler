// ---------------------------------------------
// Test 06 - 2020-04-25 - Gustavo Casanova
// .............................................
// Temperature display tests
// ---------------------------------------------


#include "victoria-control.h"

// Main function
int main(void) {
    /* ___________________
	  |					  | 
	  |	   Setup Block	  |
	  |___________________|
	*/

    SerialInit();   // Initialize USART for serial communications (57600, N, 8, 1)

    ClrScr();
    SerialTxStr(str_crlf);

    for (uint16_t i = 1023; i != 0; i--) {
        
        int temperature = GetNtcTemperature(i, TO_CELSIUS, DT_CELSIUS);
        
        SerialTxStr(str_adcout);
        SerialTxNum(i, 4);

        SerialTxChr(32);
        SerialTxChr(32);
        
        SerialTxStr(str_tempdecs);
        
        if (temperature != -32767) {
            SerialTxTemp(temperature);
            SerialTxStr(str_tempsym);
        } else {
            SerialTxStr(str_temperr);
        }
        
        SerialTxStr(str_crlf);

    }

    SerialTxStr(str_crlf);

    /* ___________________
	  |					  | 
	  |		Main Loop	  |
	  |___________________|
	*/
    for (;;) {

    }

    return 0;
    
}
