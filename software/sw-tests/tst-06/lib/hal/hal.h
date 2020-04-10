/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: hal.h (boiler hardware abstraction layer headers)
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-10 (Easter quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef _HAL_
#define _HAL_

#include <avr/pgmspace.h>
#include <stdbool.h>
#include <temp-calc.h>
#include <timers.h>

#include "../../include/hw-mapping.h"
#include "../../include/sys-types.h"

#define VALVES 3                                /* Number of gas modulator valves */
#define VALVE_OPEN_TIMER_ID 1                   /* Valve open timer id */
#define VALVE_OPEN_TIMER_DURATION 0             /* Valve open timer time-lapse */
#define VALVE_OPEN_TIMER_MODE RUN_ONCE_AND_HOLD /* Valve open timer mode */

#define DLY_DEBOUNCE_CH_REQ 1000 /* Debounce delay for CH request thermostat switch */
#define DLY_DEBOUNCE_AIRFLOW 10  /* Debounce delay for airflow sensor switch */

// Types
typedef struct debounce_sw {
    uint16_t ch_request_deb; /* CH request switch debouncing delay */
    uint16_t airflow_deb;    /* Airflow sensor switch debouncing delay*/
} DebounceSw;

typedef enum hw_switch {
    TURN_ON = 1,
    TURN_OFF = 0
} HwSwitch;

void InitDigitalSensor(SysInfo *, InputFlag);
bool CheckDigitalSensor(SysInfo *, InputFlag, DebounceSw *, bool);
void InitAnalogSensor(SysInfo *, AnalogInput);
uint16_t CheckAnalogSensor(SysInfo *, AdcBuffers *, AnalogInput, bool);
void InitActuator(SysInfo *, OutputFlag);
void ControlActuator(SysInfo *, OutputFlag, HwSwitch, bool);
void GasOff(SysInfo *);

#endif /* _HAL_ */
