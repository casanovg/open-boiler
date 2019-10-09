/*
 *  Roca Victoria 20/20T exhaust fan control
 *  Author: Gustavo Casanova / Nicebots
 *  ......................................................
 *  File: main.cpp (Application source)
 *  ......................................................
 *  Version: 1.0 / 2019-07-12
 *  gustavo.casanova@nicebots.com
 *  ......................................................
 *  This program for ATtiny85 microcontrollers replaces
 *  the factory mechanism of a Roca Victoria 20/20T
 *  (or F) to turn the exhaust fan on and off before and
 *  after a hot water request by the DHW or CH functions.
 *  The original mechanism was damaged by a relay that,
 *  after failing, burned the microcontroller output pin
 *  that handled the fan switching.
 */

#ifndef _VICTORIA_20_20T_H_
#define _VICTORIA_20_20T_H_

#ifndef __AVR_ATtiny85__
#define __AVR_ATtiny85__
#endif

// Includes
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include <stddef.h>
#include <util/delay.h>
//#include "../../nb-libs/cmd/nb-twi-cmd.h"
//#include "../../nb-libs/twis/interrupt-based/nb-usitwisl.h"

// Fan control pin
#define FAN_PIN PB0
#define FAN_DDR DDRB
#define FAN_PORT PORTB

// LED pin
#define LED_PIN PB1
#define LED_DDR DDRB
#define LED_PORT PORTB

// Domestic hot water request pin
#define DHW_PIN PB3
#define DHW_DDR DDRB
#define DHW_PORT PORTB
#define DHW_INP PINB

// Central heating request pin
#define CEH_PIN PB4
#define CEH_DDR DDRB
#define CEH_PORT PORTB
#define CEH_INP PINB

#endif /* _VICTORIA_20_20T_H_ */