/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: hal.h (boiler hardware abstraction layer headers)
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-10 (Easter Quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef _HAL_H_
#define _HAL_H_

#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <serial-ui.h>
#include <stdbool.h>
#include <temp-calc.h>
#include <timers.h>
#include <util/delay.h>

#include "../../include/errors.h"
#include "../../include/hw-mapping.h"
#include "../../include/sys-settings.h"

// Types

typedef enum hw_switch {
    TURN_ON = 1,
    TURN_OFF = 0
} HwSwitch;

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
    RingBuffer sys_mod_adc_buffer;
} AdcBuffers;

// Prototypes

void SystemRestart(void);
void InitFlags(SysInfo *p_system, FlagsType flags_type);
void SetFlag(SysInfo *p_system, FlagsType flags_type, uint8_t flag_position);
void ClearFlag(SysInfo *p_system, FlagsType flags_type, uint8_t flag_position);
void ToggleFlag(SysInfo *p_system, FlagsType flags_type, uint8_t flag_position);
bool GetFlag(SysInfo *p_system, FlagsType flags_type, uint8_t flag_position);
void InitDigitalSensor(SysInfo *p_system, InputFlag digital_sensor);
bool CheckDigitalSensor(SysInfo *p_system, InputFlag digital_sensor, bool show_dashboard);
void InitAnalogSensor(SysInfo *p_system, AnalogInput analog_sensor);
uint16_t CheckAnalogSensor(SysInfo *p_system, AdcBuffers *p_buffer_pack, AnalogInput analog_sensor, bool show_dashboard);
void InitActuator(SysInfo *p_system, OutputFlag device_flag);
void ControlActuator(SysInfo *p_system, OutputFlag device_flag, HwSwitch command, bool show_dashboard);
void InitAdcBuffers(AdcBuffers *p_buffer_pack, uint8_t buffer_length);
uint16_t AverageAdc(uint16_t adc_buffer[], uint8_t buffer_len, uint8_t start, AverageType average_type);
uint8_t GetKnobPosition(int16_t pot_adc_value, uint8_t knob_steps);
void OpenHeatValve(SysInfo *p_system, HeatValve valve_to_open);
//void ModulateHeat(SysInfo *p_system, uint16_t potentiometer_readout, uint8_t potentiometer_steps, uint32_t heat_cycle_time);
void ModulateHeat(SysInfo *p_system, uint8_t heat_level_ix, uint32_t heat_cycle_time);
void GasOff(SysInfo *p_system);

// Globals

// Heat levels valve settings
// ....................................................
// { { %valve-1, %valve-3, %valve-3 }, Kcal/h, G20_m3 }
// ....................................................
static const HeatLevel __flash heat_level[] = {
    {{100, 0, 0}, 7000, 0.870},    // Heat level 0 = 7000 Kcal/h
    {{83, 17, 0}, 7833, 0.968},    // Heat level 1 = 7833 Kcal/h
    {{67, 33, 0}, 8667, 1.067},    // Heat level 2 = 8667 Kcal/h
    {{83, 0, 17}, 9167, 1.123},    // Heat level 3 = 9167 Kcal/h
    {{50, 50, 0}, 9500, 1.165},    // Heat level 4 = 9500 Kcal/h
    {{67, 17, 16}, 10000, 1.222},  // Heat level 5 = 10000 Kcal/h
    {{33, 67, 0}, 10333, 1.263},   // Heat level 6 = 10333 Kcal/h
    {{50, 33, 17}, 10833, 1.320},  // Heat level 7 = 10833 Kcal/h
    {{17, 83, 0}, 11167, 1.362},   // Heat level 8 = 11167 Kcal/h
    {{67, 0, 33}, 11333, 1.377},   // Heat level 9 = 11333 Kcal/h
    {{33, 50, 17}, 11667, 1.418},  // Heat level 10 = 11667 Kcal/h
    {{0, 100, 0}, 12000, 1.460},   // Heat level 11 = 12000 Kcal/h
    {{50, 17, 33}, 12167, 1.475},  // Heat level 12 = 12167 Kcal/h
    {{17, 67, 16}, 12500, 1.517},  // Heat level 13 = 12500 Kcal/h
    {{34, 33, 33}, 13000, 1.573},  // Heat level 14 = 13000 Kcal/h
    {{0, 83, 17}, 13333, 1.615},   // Heat level 15 = 13333 Kcal/h
    {{50, 0, 50}, 13500, 1.630},   // Heat level 16 = 13500 Kcal/h
    {{17, 50, 33}, 13833, 1.672},  // Heat level 17 = 13833 Kcal/h
    {{33, 17, 50}, 14333, 1.728},  // Heat level 18 = 14333 Kcal/h
    {{0, 67, 33}, 14667, 1.770},   // Heat level 19 = 14667 Kcal/h
    {{17, 33, 50}, 15167, 1.827},  // Heat level 20 = 15167 Kcal/h
    {{33, 0, 67}, 15667, 1.883},   // Heat level 21 = 15667 Kcal/h
    {{0, 50, 50}, 16000, 1.925},   // Heat level 22 = 16000 Kcal/h
    {{17, 17, 66}, 16500, 1.982},  // Heat level 23 = 16500 Kcal/h
    {{0, 33, 67}, 17333, 2.080},   // Heat level 24 = 17333 Kcal/h
    {{17, 0, 83}, 17833, 2.137},   // Heat level 25 = 17833 Kcal/h
    {{0, 17, 83}, 18667, 2.235},   // Heat level 26 = 18667 Kcal/h
    {{0, 0, 100}, 20000, 2.390}    // Heat level 27 = 20000 Kcal/h
};

#endif  // _HAL_H_
