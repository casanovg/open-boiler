/*
    Roca Victoria 20/20T Boiler AC Motor Control
 */

#ifndef _VICTORIA_20_20T_H_
#define _VICTORIA_20_20T_H_

//#ifndef __AVR_ATtiny85__
//#define __AVR_ATtiny85__
//#endif

// Includes
#include <Arduino.h>
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

// Pump control pin
#define FAN_PIN PB1
#define FAN_DDR DDRB
#define FAN_PORT PORTB

// UI LED pin
#define LED_PIN PB2
#define LED_DDR DDRB
#define LED_PORT PORTB

// Domestic hot water request pin
#define DHW_PIN PC0
#define DHW_DDR DDRC
#define DHW_PORT PORTC
#define DHW_INP PINC

// Central heating request pin
#define CEH_PIN PC1
#define CEH_DDR DDRC
#define CEH_PORT PORTC
#define CEH_INP PINC

#endif /* _VICTORIA_20_20T_H_ */