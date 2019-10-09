/*
 *  Open-Boiler Control
 *  ....................................
 *  v0.3 / 2019-08-06 / ATmega328
 *  ....................................
 *  Gustavo Casanova / Nicebots
 */

//#include <Arduino.h>
//#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <string.h>


#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#define BAUDRATE 9600
#define BAUD_PRESCALER (((F_CPU / (BAUDRATE * 16UL))) - 1)

#define ERR_IGNITION 3000
#define SPARK_DELAY 1700000
#define VALVE_1_DELAY 500000

// Led UI (mini-pro pin 13 - onboard led - output)
#define LED_UI_DDR DDRB
#define LED_UI_PIN PIN5
#define LED_UI_PINP PINB
#define LED_UI_PORT PORTB
// Domestic hot water request (mini-pro pin 11 - input)
#define DHW_RQ_DDR DDRB
#define DHW_RQ_PIN PIN3
#define DHW_RQ_PINP PINB
#define DHW_RQ_PORT PORTB
// Flame detector (mini-pro pin 2 - input)
#define FLAME_DDR DDRD
#define FLAME_PIN PIN2
#define FLAME_PINP PIND
#define FLAME_PORT PORTD
// Exhaust fan (mini-pro pin 3 - output)
#define FAN_DDR DDRD
#define FAN_PIN PIN3
#define FAN_PINP PIND
#define FAN_PORT PORTD
// Spark igniter (mini-pro pin 4 - output)
#define SPARK_DDR DDRD
#define SPARK_PIN PIN4
#define SPARK_PINP PIND
#define SPARK_PORT PORTD
// Valve security (mini-pro pin 5 - output)
#define VALVE_S_DDR DDRD
#define VALVE_S_PIN PIN5
#define VALVE_S_PINP PIND
#define VALVE_S_PORT PORTD
// Valve 1 (mini-pro pin 6 - output)
#define VALVE_1_DDR DDRD
#define VALVE_1_PIN PIN6
#define VALVE_1_PINP PIND
#define VALVE_1_PORT PORTD
// Valve 2 (mini-pro pin 7 - output)
#define VALVE_2_DDR DDRD
#define VALVE_2_PIN PIN7
#define VALVE_2_PINP PIND
#define VALVE_2_PORT PORTD
// Valve 3 (mini-pro pin 8 - output)
#define VALVE_3_DDR DDRB
#define VALVE_3_PIN PIN0
#define VALVE_3_PINP PINB
#define VALVE_3_PORT PORTB

// Prototypes
void SerialInit(void);
unsigned char SerialRXChr(void);
void SerialTXChr(unsigned char data);
void SerialTXStr(char* ptr_string);
void SerialTXWrd(uint16_t data);
void ShowError(int delay_millis);
void ClrScr(void);
void AllOff(void);

// Globals
/* str_string[] is in fact an array but when we put the text between the " " symbols the compiler threats
 * it as a string and automatically puts the null termination character in the end of the text.
 */
const char __flash str_header_01[] = { "\n\rOpen-Boiler v0.3 (2019-08-06)\n\r" };
const char __flash str_header_02[] = { "\n\rHi Juan, Sandra & Gustavo!\n\r" };
const char __flash str_header_03[] = { "\n\r--> System ready\n\n\r" };
const char __flash str_flame_01[] = { "\n\r>>> Flame detected\n\r" };
const char __flash str_flame_02[] = { "\n\r>>> No flame detected\n\r" };
const char __flash str_dhw_rq_01[] = { "\n\r>>> Domestic Hot Water requested!\n\r" };
const char __flash str_dhw_rq_02[] = { "\n\r>>> End of DHW request!\n\r" };
const char __flash str_fan_01[] = { "\n\r--> Turning fan on\n\r" };
const char __flash str_fan_02[] = { "\n\r--> Turning fan off\n\r" };
const char __flash str_valve_s_01[] = { "\n\r--> Security valve open\n\r" };
const char __flash str_valve_s_02[] = { "\n\r--> Security valve closed\n\r" };
const char __flash str_valve_1_01[] = { "\n\r--> Gas valve 1 open\n\r" };
const char __flash str_valve_1_02[] = { "\n\r--> Gas valve 1 closed\n\r" };
const char __flash str_valve_2_01[] = { "\n\r--> Gas valve 2 open\n\r" };
const char __flash str_valve_2_02[] = { "\n\r--> Gas valve 2 closed\n\r" };
const char __flash str_spark_01[] = { "\n\r--> Spark igniter on\n\r" };
const char __flash str_spark_02[] = { "\n\r--> Spark igniter off\n\r" };
const char __flash str_dhw_duty_01[] = { "\n\r--> Boiler on DHW duty!\n\r" };
const char __flash str_dhw_duty_02[] = { "\n\r--> End of boiler DHW duty\n\r" };
const char __flash str_gas_off[] = { "\n\r--> Closing all gas valves\n\r" };
const char __flash str_ignition_err[] = { "Ignition error! " };
const char __flash str_parameter[] = { "Parameter: " };
const char __flash crlf[] = { "\r\n" };

long spark_delay = SPARK_DELAY;
long valve_1_delay = VALVE_1_DELAY;
bool flame = false;
bool sparks = false;
bool fan = false;
bool valve_s = false;
bool valve_1 = false;
bool valve_2 = false;
bool valve_3 = false;
bool on_duty = false;
bool dhw_req = false;

// Main function
int main(void) {

    /*  ___________________
       |                   | 
       |    Setup Block    |
       |___________________|
    */
    LED_UI_DDR |= (1 << LED_UI_PIN);        /* Set LED UI pin as output */
    LED_UI_PORT &= ~(1 << LED_UI_PIN);      /* Set LED UI pin low (inactive) */

    DHW_RQ_DDR &= ~(1 << DHW_RQ_PIN);       /* Set DHW request pin as input */
    DHW_RQ_PORT |= (1 << DHW_RQ_PIN);       /* Activate pull-up resistor on this pin */

    FLAME_DDR &= ~(1 << FLAME_PIN);         /* Set Flame detector pin as input */
    FLAME_PORT &= ~(1 << FLAME_PIN);        /* Deactivate pull-up resistor on this pin */

    FAN_DDR |= (1 << FAN_PIN);              /* Set exhaust fan pin as output */
    FAN_PORT &= ~(1 << FAN_PIN);            /* Set exhaust fan pin low (inactive) */

    SPARK_DDR |= (1 << SPARK_PIN);          /* Set spark igniter pin as output */
    SPARK_PORT |= (1 << SPARK_PIN);         /* Set spark igniter pin high (inactive) */

    VALVE_S_DDR |= (1 << VALVE_S_PIN);      /* Set security valve pin as output */
    VALVE_S_PORT &= ~(1 << VALVE_S_PIN);    /* Set security valve pin low (inactive) */

    VALVE_1_DDR |= (1 << VALVE_1_PIN);      /* Set valve 1 output pin as output */
    VALVE_1_PORT &= ~(1 << VALVE_1_PIN);    /* Set valve 1 output pin low (inactive) */

    VALVE_2_DDR |= (1 << VALVE_2_PIN);      /* Set valve 2 output pin as output */
    VALVE_2_PORT &= ~(1 << VALVE_2_PIN);    /* Set valve 2 output pin low (inactive) */

    VALVE_3_DDR |= (1 << VALVE_3_PIN);      /* Set valve 3 output pin as output */
    VALVE_3_PORT &= ~(1 << VALVE_3_PIN);    /* Set valve 3 output pin low (inactive) */

    SerialInit();  //Call the USART initialization code

    _delay_ms(1000);
    ClrScr();
    SerialTXStr(str_header_01);
    SerialTXStr(str_header_02);
    SerialTXStr(str_header_03);

    // for (int z = 0; z < 32768; z++ ) {
    //     _delay_ms(1000);
    //     ClrScr();
    //     SerialTXStr(str_header_01);
    //     SerialTXStr(str_header_02);
    //     SerialTXStr(str_header_03);
    //     //_delay_ms(500);
    //     SerialTXStr(str_parameter);
    //     SerialTXWrd(z);
    //     SerialTXStr(crlf);
    // }

    /*  ___________________
       |                   | 
       |     Main Loop     |
       |___________________|
    */
    for(;;) {

        // Check flame detector state
        if (FLAME_PINP & (1 << FLAME_PIN)) {
            if (flame == false) {
                SerialTXStr(str_flame_01);
                LED_UI_PORT |= (1 << LED_UI_PIN);
                flame = true;
            }
        } else {
            if (flame == true) {
                SerialTXStr(str_flame_02);
                LED_UI_PORT &= ~(1 << LED_UI_PIN);
                flame = false;
            }
        }

        // Check domestic hot water request detector
        if ((DHW_RQ_PINP & (1 << DHW_RQ_PIN)) == 0) {

            if (dhw_req == false) {
                dhw_req = true;
                SerialTXStr(str_dhw_rq_01);
            }

            if (flame == false) {

                if (fan == false) {
                    FAN_PORT |= (1 << FAN_PIN);
                    SerialTXStr(str_fan_01);
                    fan = true;
                    _delay_ms(2000);
                }

                if (valve_s == false) {
                    VALVE_S_PORT |= (1 << VALVE_S_PIN);
                    SerialTXStr(str_valve_s_01);
                    valve_s = true;
                    _delay_ms(500);
                }

                if (valve_1 == false) {
                    VALVE_1_PORT |= (1 << VALVE_1_PIN);
                    SerialTXStr(str_valve_1_01);
                    valve_1 = true;
                    _delay_ms(500);
                }

                if (sparks == false) {
                    SPARK_PORT &= ~(1 << SPARK_PIN);    /* Set spark output pin low (active) */
                    SerialTXStr(str_spark_01);
                    sparks = true;
                }

                if ((sparks == true) & (spark_delay-- == 0)) {
                    ShowError(ERR_IGNITION);                    /* ERROR: FLAME DIDN'T IGNITE */
                }

            } else {

                SPARK_PORT |= (1 << SPARK_PIN);
                sparks = false;
                spark_delay = SPARK_DELAY;

                if (valve_1_delay-- == 0) {
                    if (valve_1 == true) {
                        VALVE_1_PORT &= ~(1 << VALVE_1_PIN);
                        SerialTXStr(str_valve_1_02);
                        valve_1 = false;
                    }
                    if (valve_2 == false) {
                        VALVE_2_PORT |= (1 << VALVE_2_PIN);
                        SerialTXStr(str_valve_2_01);
                        SerialTXStr(str_dhw_duty_01);
                        valve_2 = true;
                        valve_1_delay = VALVE_1_DELAY;
                    }
                }
            }

        } else {

            if (dhw_req == true) {
                dhw_req = false;
                SerialTXStr(str_dhw_rq_02);
                AllOff();
                SerialTXStr(str_dhw_duty_02);
                valve_1_delay = VALVE_1_DELAY;                
            }            
        }
    }

    return 0;
}

/*   ___________________________
    |                           | 
    |     All actuators off     |
    |___________________________|
*/
void AllOff(void) {

    if (sparks == true) {
        SerialTXStr(str_spark_02);
        SPARK_PORT |= (1 << SPARK_PIN);
        sparks = false;
    }

    SerialTXStr(str_gas_off);

    VALVE_3_PORT &= ~(1 << VALVE_3_PIN);
    valve_3 = false;

    VALVE_2_PORT &= ~(1 << VALVE_2_PIN);
    valve_2 = false;

    VALVE_1_PORT &= ~(1 << VALVE_1_PIN);
    valve_1 = false;

    VALVE_S_PORT &= ~(1 << VALVE_S_PIN);
    valve_s = false;

    SerialTXStr(str_fan_02);

    FAN_PORT &= ~(1 << FAN_PIN);
    fan = false;
}

// Function SerialInit
void SerialInit(void) {
    UBRR0H = (uint8_t)(BAUD_PRESCALER >> 8);
    UBRR0L = (uint8_t)(BAUD_PRESCALER);
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (3 << UCSZ00);
}

// Function SerialRXChr
unsigned char SerialRXChr(void) {
    while (!(UCSR0A & (1 << RXC0))) {};
    return UDR0;
}

// Function SerialTXChr
void SerialTXChr(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0))) {};
    UDR0 = data;
}

// Function SerialTXWrd
void SerialTXWrd(uint16_t data) {
    #define DATA_LNG 5
    char str[DATA_LNG] = { 0 };
    uint8_t non_zero = 0;
    sprintf(str, "%02d", data);
    for (int i = 0; i < DATA_LNG; i++) {
        while (!(UCSR0A & (1 << UDRE0))) {};
        UDR0 = str[i];
    } 
}

// Function SerialTXStr (old)
// void SerialTXStr(char* ptr_string) {
//     while (*ptr_string != 0x00) {
//         SerialTXChr(*ptr_string);
//         ptr_string++;
//     }
// }

// Function SerialTXStr
void SerialTXStr(char* ptr_string) {
    for (uint8_t k = 0; k < strlen_P(ptr_string); k++) {
        char my_char = pgm_read_byte_near(ptr_string + k);
        SerialTXChr(my_char);
    }
}

// Function ShowError
void ShowError(int delay_millis) {
    AllOff();
    for (;;) {
        LED_UI_PORT ^= (1 << LED_UI_PIN);
        SerialTXStr(str_ignition_err);
        _delay_ms(delay_millis);
    }
}

// Function clear screen
void ClrScr(void) {
    SerialTXChr(27);    // ESC
    SerialTXChr(91);    // [
    SerialTXChr(50);    // 2
    SerialTXChr(74);    // J
    SerialTXChr(27);    // ESC
    SerialTXChr(91);    // 2
    SerialTXChr(72);    // H
}
