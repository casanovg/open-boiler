/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: victoria-control.h (headers) for ATmega328
 *  ........................................................
 *  Version: 0.8 "Juan" / 2019-10-11 (News)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef _VICTORIA_CONTROL_H_
#define _VICTORIA_CONTROL_H_

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <hal.h>
#include <serial-ui.h>
#include <stdbool.h>
#include <stdio.h>
#include <temp-calc.h>
#include <timers.h>
#include <util/delay.h>

#include "errors.h"
#include "hw-mapping.h"
#include "sys-types.h"

#define CH_SETPOINT_HIGH 241 /* ADC-NTC CH temperature ~ 55°C */
#define CH_SETPOINT_LOW 379  /* ADC-NTC CH temperature ~ 38°C */

#define HEAT_CYCLE_TIME 1000 /* Heat modulator cycle time (milliseconds) */

#define DHW_SETTING_STEPS 12 /* DHW setting potentiometer steps */
#define CH_SETTING_STEPS 12  /* CH setting potentiometer steps */

#ifndef MAX_IGNITION_RETRIES
#define MAX_IGNITION_RETRIES 3 /* Number of ignition retries when no flame is detected */
#endif                         /* IGNITION_RETRIES */

#ifndef OVERHEAT_OVERRIDE
#define OVERHEAT_OVERRIDE false /* True: Overheating thermostat override */
#endif                          /* OVERHEAT_OVERRIDE */

#ifndef AIRFLOW_OVERRIDE
#define AIRFLOW_OVERRIDE true /* True: Flue airflow sensor override */
#endif                        /* AIRFLOW_OVERRIDE */

#ifndef FAN_TEST_OVERRIDE
#define FAN_TEST_OVERRIDE true /* True: Flue airflow sensor override */
#endif                         /* FAN_TEST_OVERRIDE */

#ifndef FAST_FLAME_DETECTION
#define FAST_FLAME_DETECTION false /* True: Spark igniter is turned off when the flame is detected */
#endif /* FAST_FLAME_DETECTION */  /*       instead of checking the flame sensor after a delay     */

#ifndef LED_UI_FOR_FLAME
#define LED_UI_FOR_FLAME true /* True: Activates onboard LED when the flame detector is on */
#endif                        /* LED_UI_FOR_FLAME */

#ifndef SHOW_PUMP_TIMER
#define SHOW_PUMP_TIMER true /* True: Shows the CH water pump auto-shutdown timer */
#endif                       /* SHOW_PUMP_TIMER */

typedef struct heat_level {
    uint8_t valve_open_time[VALVES];
    uint16_t kcal_h;
    float gas_usage;
} HeatLevel;

typedef struct gas_modulator {
    uint8_t valve_number; /* Valve number */
    uint16_t kcal_h;      /* Kcal per hour */
    float gas_usage;      /* Gas usage per hour */
    bool status;          /* Valve status */
} GasModulator;

void InitLedUi(void);
void SetLedUiOn(void);
void SetLedUiOff(void);
void InitFlags(SysInfo *, FlagsType);
void SetFlag(SysInfo *, FlagsType, uint8_t);
void ClearFlag(SysInfo *, FlagsType, uint8_t);
bool GetFlag(SysInfo *, FlagsType, uint8_t);
void SystemRestart(void);

// Globals

// Heat levels valve settings
//const HeatLevel __flash heat_level[] = {
const HeatLevel heat_level[] = {
    /* { { %valve-1, %valve-3, %valve-3 }, Kcal/h, G20_m3 } */
    {{100, 0, 0}, 7000, 0.870},   /* Heat level 0 = 7000 Kcal/h */
    {{83, 17, 0}, 7833, 0.968},   /* Heat level 1 = 7833 Kcal/h */
    {{67, 33, 0}, 8667, 1.067},   /* Heat level 2 = 8667 Kcal/h */
    {{83, 0, 17}, 9167, 1.123},   /* Heat level 3 = 9167 Kcal/h */
    {{50, 50, 0}, 9500, 1.165},   /* Heat level 4 = 9500 Kcal/h */
    {{67, 17, 16}, 10000, 1.222}, /* Heat level 5 = 10000 Kcal/h */
    {{33, 67, 0}, 10333, 1.263},  /* Heat level 6 = 10333 Kcal/h */
    {{50, 33, 17}, 10833, 1.320}, /* Heat level 7 = 10833 Kcal/h */
    {{17, 83, 0}, 11167, 1.362},  /* Heat level 8 = 11167 Kcal/h */
    {{67, 0, 33}, 11333, 1.377},  /* Heat level 9 = 11333 Kcal/h */
    {{33, 50, 17}, 11667, 1.418}, /* Heat level 10 = 11667 Kcal/h */
    {{0, 100, 0}, 12000, 1.460},  /* Heat level 11 = 12000 Kcal/h */
    // {{50, 17, 33}, 12167, 1.475}, /* Heat level 12 = 12167 Kcal/h */
    // {{17, 67, 16}, 12500, 1.517}, /* Heat level 13 = 12500 Kcal/h */
    // {{34, 33, 33}, 13000, 1.573}, /* Heat level 14 = 13000 Kcal/h */
    // {{0, 83, 17}, 13333, 1.615},  /* Heat level 15 = 13333 Kcal/h */
    // {{50, 0, 50}, 13500, 1.630},  /* Heat level 16 = 13500 Kcal/h */
    // {{17, 50, 33}, 13833, 1.672}, /* Heat level 17 = 13833 Kcal/h */
    // {{33, 17, 50}, 14333, 1.728}, /* Heat level 18 = 14333 Kcal/h */
    // {{0, 67, 33}, 14667, 1.770},  /* Heat level 19 = 14667 Kcal/h */
    // {{17, 33, 50}, 15167, 1.827}, /* Heat level 20 = 15167 Kcal/h */
    // {{33, 0, 67}, 15667, 1.883},  /* Heat level 21 = 15667 Kcal/h */
    // {{0, 50, 50}, 16000, 1.925},  /* Heat level 22 = 16000 Kcal/h */
    // {{17, 17, 66}, 16500, 1.982}, /* Heat level 23 = 16500 Kcal/h */
    // {{0, 33, 67}, 17333, 2.080},  /* Heat level 24 = 17333 Kcal/h */
    // {{17, 0, 83}, 17833, 2.137},  /* Heat level 25 = 17833 Kcal/h */
    // {{0, 17, 83}, 18667, 2.235},  /* Heat level 26 = 18667 Kcal/h */
    // {{0, 0, 100}, 20000, 2.390}   /* Heat level 27 = 20000 Kcal/h */
};

#endif /* _VICTORIA_CONTROL_H_ */
