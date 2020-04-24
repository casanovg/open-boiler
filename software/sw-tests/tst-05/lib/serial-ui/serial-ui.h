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

#ifndef SHOW_PUMP_TIMER
#define SHOW_PUMP_TIMER true /* True: Shows the CH water pump auto-shutdown timer */
#endif                       /* SHOW_PUMP_TIMER */

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
    FLOAT_TEMP = 8,
    DIGITS_FREE = 0
} DigitLength;

// Prototypes

void SerialInit(void);
unsigned char SerialRxChr(void);
void SerialTxChr(unsigned char);
void SerialTxNum(uint32_t, DigitLength);
void SerialTxStr(const __flash char *);
void ClrScr(void);
void Dashboard(SysInfo *, bool);
void DrawLine(uint8_t, char);

// Global console UI literals

// static const char __flash str_vs[] = {" V-S "};
// static const char __flash str_v1[] = {" V-1 "};
// static const char __flash str_v2[] = {" V-2 "};
// static const char __flash str_v3[] = {" V-3 "};
// static const char __flash str_spark[] = {" IGNITING "};
// static const char __flash str_fan[] = {" F "};
// static const char __flash str_pump[] = {" PUMP "};

static const char __flash str_preboot_01[] = {" > Initializing ADC buffers ..."};
static const char __flash str_preboot_02[] = {" > Initializing actuator controls ..."};
static const char __flash str_preboot_03[] = {" > Turning all actuators off ..."};
static const char __flash str_preboot_04[] = {" > Initializing digital sensor flags ..."};
static const char __flash str_preboot_05[] = {" > Initializing analog sensor inputs ..."};
static const char __flash str_preboot_06[] = {" > Pre-loading analog sensor values ..."};
static const char __flash str_preboot_07[] = {" Starting normal FSM cycle ..."};
static const char __flash str_header_01[] = {" " FW_NAME " " FW_VERSION " "};
static const char __flash str_header_02[] = {FW_ALIAS};
static const char __flash str_heat_mod_01[] = {" Heat Mode: "};
static const char __flash str_heat_mod_02[] = {" (OK)"};
static const char __flash str_heat_mod_03[] = {" (VALVE TIME ERROR)"};
static const char __flash str_heat_mod_04[] = {"V-"};
static const char __flash str_heat_mod_05[] = {" | (O)"};
static const char __flash str_heat_mod_06[] = {" | (X)"};
static const char __flash str_heat_mod_07[] = {" | (=)"};
static const char __flash str_heat_mod_08[] = {" ms "};

static const char __flash str_iflags[] = {"Inputs: "};
static const char __flash str_oflags[] = {"Outputs: "};
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
static const char __flash str_crlf[] = {"\r\n"};
static const char __flash str_error_s[] = {"                        >>> Error "};
static const char __flash str_error_e[] = {" <<<"};
static const uint8_t __flash clr_ascii[] = {27, 91, 50, 74, 27, 91, 72};
static const char __flash sys_mode_00[] = {"[ WINTER ] "};
static const char __flash sys_mode_01[] = {"[ SUMMER ] "};
static const char __flash sys_mode_02[] = {"[ PWR-OFF ]"};
static const char __flash sys_mode_03[] = {"[ RESET ]  "};
static const char __flash str_mode_00[] = {"          [ OFF ] .\n\r"};
static const char __flash str_mode_10[] = {"        [ READY ] .\n\r"};
static const char __flash str_mode_20[] = {"     [ IGNITING ] .\n\r"};
static const char __flash str_mode_30[] = {"  [ DHW ON DUTY ] .\n\r"};
static const char __flash str_mode_40[] = {"   [ CH ON DUTY ] .\n\r"};
static const char __flash str_mode_100[] = {"        [ ERROR ] .\n\r"};
#if SHOW_PUMP_TIMER
static const char __flash str_wptimer[] = {"  CH water pump auto-shutdown timer: "};
#endif /* SHOW_PUMP_TIMER */
//static const char __flash str_bug[] = {"  FORCED BUG !!! "};

#endif /* _SERIAL_UI_H_ */
