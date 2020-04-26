/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: serial-ui.h (serial user interface headers)
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-09 (Easter Quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef _SERIAL_UI_H_
#define _SERIAL_UI_H_

#include <avr/pgmspace.h>
#include <hal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <temp-calc.h>
#include <timers.h>

#include "../../include/sys-settings.h"

// Dashboard language available
#define _EN_ 1  // English dashboard
#define _ES_ 2  // Spanish dashboard
// Dashboard language selection
#if DASHBOARD_LANG == _EN_
#include "dashboard-en.h"
#elif DASHBOARD_LANG == _ES_
#include "dashboard-es.h"
#else
#include "dashboard-en.h"
#endif  // DASHBOARD_LANG

// Serial comm settings

#define BAUDRATE 57600
#define BAUD_PRESCALER (((F_CPU / (BAUDRATE * 16UL))) - 1)

// Types

typedef enum digit_length {
    DIGITS_1 = 1,
    DIGITS_2 = 2,
    DIGITS_3 = 3,
    DIGITS_4 = 4,
    DIGITS_5 = 5,
    DIGITS_6 = 6,
    DIGITS_7 = 7,
    DIGITS_8 = 8,
    DIGITS_9 = 9,
    DIGITS_10 = 10,
    DIGITS_FREE = 0
} DigitLength;

// Prototypes

void SerialInit(void);
uint8_t SerialRxChr(void);
void SerialTxChr(uint8_t character_code);
void SerialTxNum(uint32_t number, DigitLength digits);
void SerialTxStr(const __flash char *ptr_string);
void SerialTxTemp(int ntc_temperature);
void DrawLine(uint8_t length, char line_char);
int DivRound(const int numerator, const int denominator);
void ClrScr(void);
#if SHOW_DASHBOARD
void Dashboard(SysInfo *p_system, bool force_refresh);
#endif  // SHOW_DASHBOARD

// Global console UI literals

static const char __flash str_header_01[] = {" " FW_NAME " " FW_VERSION " "};
static const uint8_t __flash clr_ascii[] = {27, 91, 50, 74, 27, 91, 72};
static const char __flash str_crlf[] = {"\r\n"};

#endif  // _SERIAL_UI_H_
