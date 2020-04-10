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

#define DHW_SETTING_STEPS 12 /* DHW setting potentiometer steps */
#define CH_SETTING_STEPS 12  /* CH setting potentiometer steps */

#define VALVES 3                                /* Number of gas modulator valves */
#define VALVE_OPEN_TIMER_ID 1                   /* Valve open timer id */
#define VALVE_OPEN_TIMER_DURATION 0             /* Valve open timer time-lapse */
#define VALVE_OPEN_TIMER_MODE RUN_ONCE_AND_HOLD /* Valve open timer mode */

#define DLY_DEBOUNCE_CH_REQ 1000 /* Debounce delay for CH request thermostat switch */
#define DLY_DEBOUNCE_AIRFLOW 10  /* Debounce delay for airflow sensor switch */

// Types
typedef enum hw_switch {
    TURN_ON = 1,
    TURN_OFF = 0
} HwSwitch;

typedef struct debounce_sw {
    uint16_t ch_request_deb; /* CH request switch debouncing delay */
    uint16_t airflow_deb;    /* Airflow sensor switch debouncing delay*/
} DebounceSw;

typedef enum average_type {
    MEAN = 0,
    ROBUST = 1,
    MOVING = 2
} AverageType;

typedef struct ring_buffer {
    uint16_t data[BUFFER_LENGTH];
    uint8_t ix;
} RingBuffer;

typedef struct adc_buffers {
    RingBuffer dhw_temp_adc_buffer;
    RingBuffer ch_temp_adc_buffer;
    RingBuffer dhw_set_adc_buffer;
    RingBuffer ch_set_adc_buffer;
    RingBuffer sys_set_adc_buffer;
} AdcBuffers;

typedef struct gas_valve {
    uint8_t valve_number; /* Valve number */
    uint16_t kcal_h;      /* Kcal per hour */
    float gas_usage;      /* Gas usage per hour */
    bool status;          /* Valve status */
} GasValve;

void InitDigitalSensor(SysInfo *, InputFlag);
bool CheckDigitalSensor(SysInfo *, InputFlag, DebounceSw *, bool);
void InitAnalogSensor(SysInfo *, AnalogInput);
uint16_t CheckAnalogSensor(SysInfo *, AdcBuffers *, AnalogInput, bool);
void InitActuator(SysInfo *, OutputFlag);
void ControlActuator(SysInfo *, OutputFlag, HwSwitch, bool);
void InitAdcBuffers(AdcBuffers *, uint8_t);
uint16_t AverageAdc(uint16_t[], uint8_t, uint8_t, AverageType);
uint8_t GetHeatLevel(int16_t, uint8_t);
void GasOff(SysInfo *);

#endif /* _HAL_ */
