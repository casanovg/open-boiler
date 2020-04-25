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
#include <temp-calc.h>
#include <timers.h>

#include "../../include/sys-settings.h"

// Serial comm settings

#define BAUDRATE 57600
#define BAUD_PRESCALER (((F_CPU / (BAUDRATE * 16UL))) - 1)

// Dashboard defines

#define DASH_WIDTH 65
#define H_ELINE 46
#define H_ILINE 46
#define V_LINE 46
#define SPACE 32

#define SHOW_PUMP_TIMER true  // True: Shows the CH water pump auto-shutdown timer

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
    TEMP_NN = 50,
    TEMP_DD = 51,
    ZUZU = 66,
    DIGITS_FREE = 0
} DigitLength;

// Prototypes

void SerialInit(void);
uint8_t SerialRxChr(void);
void SerialTxChr(uint8_t character_code);
void SerialTxNum(uint32_t number, DigitLength digits);
void SerialTxStr(const __flash char *ptr_string);
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

static const char __flash str_adcout[] = {"ADC output: "};
static const char __flash str_tempdecs[] = {"Temp decs = "};
static const char __flash str_tempvalue[] = {"Temp value = "};
static const char __flash str_temperr[] = {"----"};
static const char __flash str_tempsym[] = {"'C"};

#if SHOW_DASHBOARD
static const char __flash str_header_02[] = {FW_ALIAS};
static const char __flash str_iflags[] = {"IF: "};
static const char __flash str_oflags[] = {"OF: "};
static const char __flash str_lit_00[] = {"DHW request: "};
static const char __flash str_lit_01[] = {"CH request: "};
static const char __flash str_lit_02[] = {" Airflow: "};
static const char __flash str_lit_02_override[] = {"!Airflow: "};
static const char __flash str_lit_03[] = {"Flame: "};
static const char __flash str_lit_04[] = {" Overheat: "};
static const char __flash str_lit_04_override[] = {"!Overheat: "};
static const char __flash str_lit_05[] = {"Fan: "};
static const char __flash str_lit_06[] = {"Pump: "};
static const char __flash str_lit_07[] = {"Igniter: "};
static const char __flash str_lit_08[] = {"Security valve: "};
static const char __flash str_lit_09[] = {"Valve-1: "};
static const char __flash str_lit_10[] = {"Valve-2: "};
static const char __flash str_lit_11[] = {"Valve-3: "};
static const char __flash str_lit_12[] = {"LED UI: "};
static const char __flash str_lit_13[] = {"DHW temp: "};
static const char __flash str_lit_14[] = {"CH temp: "};
static const char __flash str_lit_15[] = {"DHW set: "};
static const char __flash str_lit_16[] = {"CH set: "};
static const char __flash str_lit_17[] = {"Sys set: "};
static const char __flash str_lit_18[] = {"Settings -> "};
static const char __flash str_true[] = {"Yes"};
static const char __flash str_false[] = {"No "};
static const char __flash str_error_s[] = {"                        >>> Error "};
static const char __flash str_error_e[] = {" <<<"};
static const char __flash sys_mode_00[] = {"[ WINTER ] "};
static const char __flash sys_mode_01[] = {"[ SUMMER ] "};
static const char __flash sys_mode_02[] = {"[ PWR-OFF ]"};
static const char __flash sys_mode_03[] = {"[ RESET ]  "};
static const char __flash str_mode_00[] = {"          [ OFF ] .\n\r"};
static const char __flash str_mode_10[] = {"        [ READY ] .\n\r"};
static const char __flash str_mode_20[] = {"     [ IGNITING ] .\n\r"};
static const char __flash str_mode_30[] = {"  [ DHW ON DUTY ] .\n\r"};
static const char __flash str_mode_40[] = {" [ CH ON DUTY "};
static const char __flash str_mode_41[] = {"1 ] .\n\r"};
static const char __flash str_mode_42[] = {"2 ] .\n\r"};
static const char __flash str_mode_100[] = {"        [ ERROR ] .\n\r"};

#if SHOW_PUMP_TIMER
static const char __flash str_wptimer[] = {"  CH water pump auto-shutdown timer: "};
static const char __flash str_wpmemory[] = {"<- Remaining time in memory: "};
//static const char __flash str_temperr[] = {"XX.X"};
#endif  // SHOW_PUMP_TIMER

#else
static const char __flash str_no_dashboard[] = {"- System dashboard disabled in settings ..."};
#endif  // SHOW_DASHBOARD

#endif  // _SERIAL_UI_H_
