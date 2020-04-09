/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: victoria-control.h (headers) for ATmega328
 *  ........................................................
 *  Version: 0.7 "Juan" / 2019-10-11 (News)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef VICTORIA_CONTROL_H_
#define VICTORIA_CONTROL_H_

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include <stdio.h>
//#include <time.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "delays.h"
#include "errors.h"
#include "hw_mapping.h"

#include "delays.h"
#include "errors.h"
#include "hw_mapping.h"

// This software
#define FW_NAME "OPEN-BOILER"
#define FW_VERSION "v0.7x"
#define FW_ALIAS "\"eXperimental Test-04\"    "

// Serial comm settings
#define BAUDRATE 38400
#define BAUD_PRESCALER (((F_CPU / (BAUDRATE * 16UL))) - 1)

#define CH_SETPOINT_HIGH 241 /* ADC-NTC CH temperature ~ 55°C */
#define CH_SETPOINT_LOW 379  /* ADC-NTC CH temperature ~ 38°C */

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

#define BUFFER_LENGTH 34 /* Circular buffers length */

#define DHW_SETTING_STEPS 12 /* DHW setting potentiometer steps */
#define CH_SETTING_STEPS 12  /* CH setting potentiometer steps */

#define VALVES 3 /* Number of system valves for heat modulation */

#define ADC_MIN 0
#define ADC_MAX 1023
#define MAX_CH_TEMP_TOLERANCE 65

#define CH_TEMP_MASK 0x3FE

// Filter settings
#define FIR_SUM 11872
#define IR_VAL 50
#define FIR_LEN 31

// Number of NTC ADC values used for calculating temperature
#define NTC_VALUES 12

// Temperature calculation settings
#define TO_CELSIUS -200   /* Celsius offset value */
#define DT_CELSIUS 100    /* Celsius delta T (difference between two consecutive table entries) */
#define TO_KELVIN 2530    /* Kelvin offset value */
#define DT_KELVIN 100     /* Kelvin delta T (difference between two consecutive table entries) */
#define TO_FAHRENHEIT -40 /* Fahrenheit offset value */
#define DT_FAHRENHEIT 180 /* Fahrenheit delta T (difference between two consecutive table entries) */

// Timer function defines
#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( ( (a)*1000L) / (F_CPU / 1000L) )
// #define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )
#define microsecondsToClockCycles(a) ( (a) * clockCyclesPerMicrosecond() )
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))
// The whole number of milliseconds per timer0 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)
// The fractional number of milliseconds per timer0 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

// Types
typedef enum states {
    OFF = 0,
    READY = 10,
    IGNITING = 20,
    DHW_ON_DUTY = 30,
    CH_ON_DUTY = 40,
    ERROR = 100
} State;

typedef enum inner_steps {
    OFF_1 = 1,
    OFF_2 = 2,
    OFF_3 = 3,
    OFF_4 = 4,
    READY_1 = 11,
    IGNITING_1 = 21,
    IGNITING_2 = 22,
    IGNITING_3 = 23,
    IGNITING_4 = 24,
    IGNITING_5 = 25,
    IGNITING_6 = 26,
    DHW_ON_DUTY_1 = 31,
    DHW_ON_DUTY_2 = 32,
    DHW_ON_DUTY_3 = 33,
    CH_ON_DUTY_1 = 41,
    CH_ON_DUTY_2 = 42
} InnerStep;

typedef enum flags_types {
    INPUT_FLAGS = 0,
    OUTPUT_FLAGS = 1
} FlagsType;

typedef enum input_flags {
    DHW_REQUEST = 0,
    CH_REQUEST = 1,
    AIRFLOW = 2,
    FLAME = 3,
    OVERHEAT = 4
} InputFlag;

typedef enum analog_inputs {
    DHW_TEMPERATURE = 6,
    CH_TEMPERATURE = 7,
    DHW_SETTING = 0,
    CH_SETTING = 1,
    SYSTEM_SETTING = 2
} AnalogInput;

typedef enum output_flags {
    EXHAUST_FAN = 0,
    WATER_PUMP = 1,
    SPARK_IGNITER = 2,
    VALVE_S = 3,
    VALVE_1 = 4,
    VALVE_2 = 5,
    VALVE_3 = 6,
    LED_UI = 7
} OutputFlag;

typedef enum hw_switch {
    TURN_ON = 1,
    TURN_OFF = 0
} HwSwitch;

typedef enum digit_length {
    DIGITS_1 = 1,
    DIGITS_2 = 2,
    DIGITS_3 = 3,
    DIGITS_4 = 4,
    DIGITS_5 = 5,
    DIGITS_6 = 6,
    DIGITS_7 = 7,
    FLOAT_TEMP = 8,
    DIGITS_FREE = 0
} DigitLength;

typedef enum average_type {
    MEAN = 0,
    ROBUST = 1,
    MOVING = 2
} AverageType;

typedef struct sys_info {
    State system_state;            /* System running state */
    InnerStep inner_step;          /* State inner step (sub-states) */
    uint8_t input_flags;           /* Flags signaling input sensor status */
    uint8_t output_flags;          /* Flags signaling hardware activation status */
    uint16_t dhw_temperature;      /* DHW NTC thermistor temperature readout */
    uint16_t ch_temperature;       /* CH NTC thermistor temperature readout */
    uint16_t dhw_setting;          /* DWH setting potentiometer readout */
    uint16_t ch_setting;           /* CH setting potentiometer readout */
    uint16_t system_setting;       /* System mode potentiometer readout */
    uint8_t last_displayed_iflags; /* Input sensor flags last shown status */
    uint8_t last_displayed_oflags; /* Hardware activation flags last shown status */
    uint8_t ignition_retries;      /* Ignition retry counter */
    uint8_t error;                 /* System error code */
    uint32_t pump_delay;           /* CH water pump auto-shutdown timer */
    InnerStep ch_on_duty_step;     /* CH inner step before handing over control to DHW */
    uint8_t dhw_heat_level;
    uint8_t ch_heat_level;
} SysInfo;

typedef struct heat_level {
    uint8_t valve_open_time[VALVES];
    uint16_t kcal_h;
    float gas_usage;
} HeatLevel;

typedef struct gas_valve {
    uint8_t valve_number; /* Valve number */
    uint16_t kcal_h;      /* Kcal per hour */
    float gas_usage;      /* Gas usage per hour */
    bool status;          /* Valve status */
} GasValve;

typedef struct debounce_sw {
    uint16_t ch_request_deb; /* CH request switch debouncing delay */
    uint16_t airflow_deb;    /* Airflow sensor switch debouncing delay*/
} DebounceSw;

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
void SerialInit(void);
unsigned char SerialRxChr(void);
void SerialTxChr(unsigned char);
void SerialTxNum(uint32_t, DigitLength);
void SerialTxStr(const __flash char *);
void ClrScr(void);
void Dashboard(SysInfo *, bool);
void DrawLine(uint8_t, char);
void InitLedUi(void);
void SetLedUiOn(void);
void SetLedUiOff(void);
void InitFlags(SysInfo *, FlagsType);
void SetFlag(SysInfo *, FlagsType, uint8_t);
void ClearFlag(SysInfo *, FlagsType, uint8_t);
bool GetFlag(SysInfo *, FlagsType, uint8_t);
void InitActuator(SysInfo *, OutputFlag);
void ControlActuator(SysInfo *, OutputFlag, HwSwitch, bool);
void InitDigitalSensor(SysInfo *, InputFlag);
bool CheckDigitalSensor(SysInfo *, InputFlag, DebounceSw *, bool);
void InitAnalogSensor(SysInfo *, AnalogInput);
uint16_t CheckAnalogSensor(SysInfo *, AdcBuffers *, AnalogInput, bool);
void GasOff(SysInfo *);
void SystemRestart(void);
void InitAdcBuffers(AdcBuffers *, uint8_t);
uint16_t AverageAdc(uint16_t[], uint8_t, uint8_t, AverageType);
uint16_t FilterFir(uint16_t[], uint8_t, uint8_t);
uint16_t FilterIir(uint16_t);
int GetNtcTemperature(uint16_t, int, int);
int GetNtcTempTenths(uint16_t, int, int);
float GetNtcTempDegrees(uint16_t, int, int);
uint8_t GetHeatLevel(int16_t, uint8_t);
void SetTickTimer(void);
unsigned long GetMilliseconds(void);

// Globals

// Timer function variables
volatile unsigned long timer0_overflow_count = 0;
volatile unsigned long timer0_milliseconds = 0;
static unsigned char timer0_fractions = 0;

// Temperature to ADC readings conversion table
//  T°C:  -20, -10,   0,  10,  20,  30,  40,  50,  60,  70,  80, 90
//  ADC:  929, 869, 787, 685, 573, 461, 359, 274, 206, 154, 116, 87
//  NTC: 98.66, 56.25, 33.21, 20.24, 12.71, 8.19, 5.42, 3.66, 2.53, 1.78, 1.28, 0.93
const uint16_t __flash ntc_adc_table[NTC_VALUES] = {
    929, 869, 787, 685, 573, 461, 359, 274, 206, 154, 116, 87};

// FIR filter value table
const uint16_t __flash fir_table[FIR_LEN] = {
    1, 3, 9, 23, 48, 89, 149, 230, 333, 454, 586, 719, 840, 938, 1002, 1024,
    1002, 938, 840, 719, 586, 454, 333, 230, 149, 89, 48, 23, 9, 3, 1};

// Heat levels valve settings
const HeatLevel __flash heat_level[] = {
    // { { %valve-1, %valve-2, %valve-3 }, Kcal/h, G20_m3 }
    {{100, 0, 0}, 7000, 0.870},   /* Heat level 0 = 7000 Kcal/h */
    {{83, 17, 0}, 7833, 0.968},   /* Heat level 1 = 7833 Kcal/h */
    {{67, 33, 0}, 8667, 1.067},   /* Heat level 2 = 8667 Kcal/h */
    {{83, 0, 17}, 9167, 1.123},   /* Heat level 3 = 9167 Kcal/h */
    {{50, 50, 0}, 9500, 1.165},   /* Heat level 4 = 9500 Kcal/h @@@@@@@@@@@ */
    {{67, 17, 16}, 10000, 1.222}, /* Heat level 5 = 10000 Kcal/h */
    {{33, 67, 0}, 10333, 1.263},  /* Heat level 6 = 10333 Kcal/h */
    {{50, 33, 17}, 10833, 1.320}, /* Heat level 7 = 10833 Kcal/h @@@@@@@@@@@ */
    {{17, 83, 0}, 11167, 1.362},  /* Heat level 8 = 11167 Kcal/h */
    {{67, 0, 33}, 11333, 1.377},  /* Heat level 9 = 11333 Kcal/h */
    {{33, 50, 17}, 11667, 1.418}, /* Heat level 10 = 11667 Kcal/h @@@@@@@@@@@ */
    {{0, 100, 0}, 12000, 1.460}   /* Heat level 11 = 12000 Kcal/h */
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

// Console UI literals
const char __flash str_header_01[] = {" " FW_NAME " " FW_VERSION " "};
const char __flash str_header_02[] = {FW_ALIAS};
const char __flash str_iflags[] = {"Inputs: "};
const char __flash str_oflags[] = {"Outputs: "};
const char __flash str_lit_00[] = {"DHW request: "};
const char __flash str_lit_01[] = {"CH request: "};
const char __flash str_lit_02[] = {" Airflow: "};
const char __flash str_lit_02_override[] = {"!Airflow: "};
const char __flash str_lit_03[] = {"Flame: "};
const char __flash str_lit_04[] = {" Overheat: "};
const char __flash str_lit_04_override[] = {"!Overheat: "};
const char __flash str_lit_05[] = {"Fan: "};
const char __flash str_lit_06[] = {"Pump: "};
const char __flash str_lit_07[] = {"Igniter: "};
const char __flash str_lit_08[] = {"Security valve: "};
const char __flash str_lit_09[] = {"Valve-1: "};
const char __flash str_lit_10[] = {"Valve-2: "};
const char __flash str_lit_11[] = {"Valve-3: "};
const char __flash str_lit_12[] = {"LED UI: "};
const char __flash str_lit_13[] = {"DHW temp: "};
const char __flash str_lit_14[] = {"CH temp: "};
const char __flash str_lit_15[] = {"DHW set: "};
const char __flash str_lit_16[] = {"CH set: "};
const char __flash str_lit_17[] = {"Sys set: "};
const char __flash str_lit_18[] = {"Settings -> "};
const char __flash str_true[] = {"Yes"};
const char __flash str_false[] = {"No "};
const char __flash str_crlf[] = {"\r\n"};
const char __flash str_error_s[] = {"                        >>> Error "};
const char __flash str_error_e[] = {" <<<"};
const uint8_t __flash clr_ascii[] = {27, 91, 50, 74, 27, 91, 72};
const char __flash str_mode_00[] = {"          [ OFF ] .\n\r"};
const char __flash str_mode_10[] = {"        [ READY ] .\n\r"};
const char __flash str_mode_20[] = {"     [ IGNITING ] .\n\r"};
const char __flash str_mode_30[] = {"  [ DHW ON DUTY ] .\n\r"};
const char __flash str_mode_40[] = {"   [ CH ON DUTY ] .\n\r"};
const char __flash str_mode_100[] = {"        [ ERROR ] .\n\r"};
#if SHOW_PUMP_TIMER
const char __flash str_wptimer[] = {"  CH water pump auto-shutdown timer: "};
#endif /* SHOW_PUMP_TIMER */
//const char __flash str_bug[] = {"  FORCED BUG !!! "};

#endif /* VICTORIA_CONTROL_H_ */
