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
#include <avr/wdt.h>
#include <stdbool.h>
#include <temp-calc.h>

#include "../../include/hw-mapping.h"
#include "../../include/sys-setup.h"

#define DLY_DEBOUNCE_CH_REQ ((uint16_t)1000) /* Debounce delay for CH request thermostat switch */
#define DLY_DEBOUNCE_AIRFLOW ((uint16_t)10)  /* Debounce delay for airflow sensor switch */

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

// Prototypes
void SystemRestart(void);
void InitFlags(SysInfo *, FlagsType);
void SetFlag(SysInfo *, FlagsType, uint8_t);
void ClearFlag(SysInfo *, FlagsType, uint8_t);
bool GetFlag(SysInfo *, FlagsType, uint8_t);
void InitDigitalSensor(SysInfo *, InputFlag);
bool CheckDigitalSensor(SysInfo *, InputFlag, DebounceSw *, bool);
void InitAnalogSensor(SysInfo *, AnalogInput);
uint16_t CheckAnalogSensor(SysInfo *, AdcBuffers *, AnalogInput, bool);
void InitActuator(SysInfo *, OutputFlag);
void ControlActuator(SysInfo *, OutputFlag, HwSwitch, bool);
void InitAdcBuffers(AdcBuffers *, uint8_t);
uint16_t AverageAdc(uint16_t[], uint8_t, uint8_t, AverageType);
uint8_t GetHeatLevel(int16_t, uint8_t);
void ModulateGas(SysInfo *, HeatValve);
//void Open exclusively();
void GasOff(SysInfo *);

#endif /* _HAL_ */
