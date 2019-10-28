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
#include <util/delay.h>
#include "delays.h"
#include "errors.h"

// Serial comm settings
#define BAUDRATE 38400
#define BAUD_PRESCALER (((F_CPU / (BAUDRATE * 16UL))) - 1)

#define CH_SETPOINT_HIGH 241        /* ADC-NTC CH temperature ~ 55°C */
#define CH_SETPOINT_LOW 379         /* ADC-NTC CH temperature ~ 38°C */

#ifndef MAX_IGNITION_RETRIES
#define MAX_IGNITION_RETRIES 3      /* Number of ignition retries when no flame is detected */
#endif /* IGNITION_RETRIES */

#ifndef OVERHEAT_OVERRIDE
#define OVERHEAT_OVERRIDE false     /* True: Overheating thermostat override */
#endif /* OVERHEAT_OVERRIDE */

#ifndef AIRFLOW_OVERRIDE
#define AIRFLOW_OVERRIDE false      /* True: Flue airflow sensor override */
#endif /* AIRFLOW_OVERRIDE */

#ifndef FAN_TEST_OVERRIDE
#define FAN_TEST_OVERRIDE false     /* True: Flue airflow sensor override */
#endif /* FAN_TEST_OVERRIDE */

#ifndef FAST_FLAME_DETECTION
#define FAST_FLAME_DETECTION false  /* True: Spark igniter is turned off when the flame is detected */
#endif /* FAST_FLAME_DETECTION */   /*       instead of checking the flame sensor after a delay     */

#ifndef LED_UI_FOR_FLAME
#define LED_UI_FOR_FLAME true       /* True: Activates onboard LED when the flame detector is on */
#endif /* LED_UI_FOR_FLAME */

#ifndef SHOW_PUMP_TIMER
#define SHOW_PUMP_TIMER true        /* True: Shows the CH water pump auto-shutdown timer */
#endif /* SHOW_PUMP_TIMER */

#define BUFFER_LENGTH 34            /* Circular buffers length */

#define DHW_SETTING_STEPS 12        /* DHW setting potentiometer steps */
#define CH_SETTING_STEPS 12         /* CH setting potentiometer steps */

// Flame detector (mini-pro pin 2 - input)
#define FLAME_DDR DDRD
#define FLAME_PIN PIN2
#define FLAME_PINP PIND
#define FLAME_PORT PORTD
// Exhaust fan (mini-pro pin 3 - output)
#define FAN_DDR DDRD
#define FAN_PIN PIN3
#define FAN_PINP PIND
#define FAN_PORT PORTD
// Spark igniter (mini-pro pin 4 - output)
#define SPARK_DDR DDRD
#define SPARK_PIN PIN4
#define SPARK_PINP PIND
#define SPARK_PORT PORTD
// Valve security (mini-pro pin 5 - output)
#define VALVE_S_DDR DDRD
#define VALVE_S_PIN PIN5
#define VALVE_S_PINP PIND
#define VALVE_S_PORT PORTD
// Valve 1 (mini-pro pin 6 - output)
#define VALVE_1_DDR DDRD
#define VALVE_1_PIN PIN6
#define VALVE_1_PINP PIND
#define VALVE_1_PORT PORTD
// Valve 2 (mini-pro pin 7 - output)
#define VALVE_2_DDR DDRD
#define VALVE_2_PIN PIN7
#define VALVE_2_PINP PIND
#define VALVE_2_PORT PORTD
// Valve 3 (mini-pro pin 8 - output)
#define VALVE_3_DDR DDRB
#define VALVE_3_PIN PIN0
#define VALVE_3_PINP PINB
#define VALVE_3_PORT PORTB
// Water pump (mini-pro pin 9 - output)
#define PUMP_DDR DDRB
#define PUMP_PIN PIN1
#define PUMP_PINP PINB
#define PUMP_PORT PORTB
// Overheat thermostat (mini-pro pin 10 - input)
#define OVERHEAT_DDR DDRB
#define OVERHEAT_PIN PIN2
#define OVERHEAT_PINP PINB
#define OVERHEAT_PORT PORTB
// Domestic Hot Water request (mini-pro pin 11 - input)
#define DHW_RQ_DDR DDRB
#define DHW_RQ_PIN PIN3
#define DHW_RQ_PINP PINB
#define DHW_RQ_PORT PORTB
// Central Heating request (mini-pro pin 12 - input)
#define CH_RQ_DDR DDRB
#define CH_RQ_PIN PIN4
#define CH_RQ_PINP PINB
#define CH_RQ_PORT PORTB
// Led UI (mini-pro pin 13 - onboard led - output)
#define LED_UI_DDR DDRB
#define LED_UI_PIN PIN5
#define LED_UI_PINP PINB
#define LED_UI_PORT PORTB
// DHW potentiometer (mini-pro pin A0 - intput)
#define DHW_POT ADC0
// CH potentiometer (mini-pro pin A1 - intput)
#define CH_POT ADC1
// System potentiometer (mini-pro pin A2 - intput)
#define SYS_POT ADC2
// Air-flow sensor (mini-pro pin A3 - intput) Board 4-6 left to right
#define AIRFLOW_DDR DDRC
#define AIRFLOW_PIN PIN3
#define AIRFLOW_PINP PINC
#define AIRFLOW_PORT PORTC
// DHW temperature sensor (mini-pro pin A6 - intput)
//#define DHW_TEMP ADC6
// CH temperature sensor (mini-pro pin A7 - intput)
//#define CH_TEMP ADC7
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
#define TO_CELSIUS -200    /* Celsius offset value */
#define DT_CELSIUS 100     /* Celsius delta T (difference between two consecutive table entries) */
#define TO_KELVIN 2530     /* Kelvin offset value */
#define DT_KELVIN 100      /* Kelvin delta T (difference between two consecutive table entries) */
#define TO_FAHRENHEIT -40 /* Fahrenheit offset value */
#define DT_FAHRENHEIT 180  /* Fahrenheit delta T (difference between two consecutive table entries) */

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
} SysInfo;

typedef struct heat_power {
    bool valve_3_state;
    bool valve_2_state;
    bool valve_1_state;
} HeatPower;

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

// Globals
const uint16_t fir_table[FIR_LEN] = {
    1, 3, 9, 23, 48, 89, 149, 230, 333, 454, 586, 719, 840, 938, 1002, 1024,
    1002, 938, 840, 719, 586, 454, 333, 230, 149, 89, 48, 23, 9, 3, 1};

/* Original article table
const uint16_t ntc_adc_temp[NTC_VALUES] = {
    939, 892, 828, 749, 657, 560, 464, 377, 300, 237, 186};
*/
const uint16_t ntc_adc_table[NTC_VALUES] = {
    929, 869, 787, 685, 573, 461, 359, 274, 206, 154, 116, 87};

 /*
 -20, -10, 0, 10, 20, 30, 40, 50, 60, 70, 80, 90
98,66	56,25	33,21	20,24	12,71	8,19	5,42	3,66	2,53	1,78	1,28	0,93
929, 869, 787, 685, 573, 461, 359, 274, 206, 154, 116, 87
 */   

const char __flash str_header_01[] = {" OPEN-BOILER v0.7   "};
const char __flash str_header_02[] = {"\"Juan, Sandra & Gustavo\" "};
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
