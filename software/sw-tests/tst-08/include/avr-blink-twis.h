/*
 *  AVR BLINK TWIS for ATmega & ATtiny85
 *  Author: Gustavo Casanova / Nicebots
 *  .............................................
 *  File: avr-blink-twis.h (Application headers)
 *  ............................................. 
 *  Version: 1.0 / 2020-05-13
 *  gustavo.casanova@nicebots.com
 *  .............................................
 */

#ifndef _AVR_BLINK_TWIS_H_
#define _AVR_BLINK_TWIS_H_

#ifdef ARDUINO_AVR_PRO
#define TWI_ADDR 12
#endif // ARDUINO_AVR_PRO

#ifdef ARDUINO_AVR_ATTINYX5
#define TWI_ADDR 13
#endif // ARDUINO_AVR_ATTINYX5

// Includes
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <nb-twi-cmd.h>
#include <stdbool.h>
#include <stddef.h>
#include <util/delay.h>

#ifdef ARDUINO_AVR_PRO
#include <nb-twisl.h>
#endif // ARDUINO_AVR_PRO

#ifdef ARDUINO_AVR_ATTINYX5
#include <nb-usitwisl.h>
//#include <nb-usitwisl-if.h>
#endif // ARDUINO_AVR_ATTINYX5

#define LED_PIN PB1
#define LED_DDR DDRB
#define LED_PORT PORTB

#endif  // _AVR_BLINK_TWIS_H_