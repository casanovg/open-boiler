/*
 *  Open-Boiler Control
 *  ....................................
 *  File: victoria-control.h
 *  ....................................
 *  v0.4 / 2019-08-08 / ATmega328
 *  ....................................
 *  Gustavo Casanova / Nicebots
 */

#ifndef VICTORIA_CONTROL_H_
#define VICTORIA_CONTROL_H_

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <util/delay.h>
// #include <string.h>
// #include <avr/wdt.h>
// #include <avr/power.h>
// #include <avr/interrupt.h>
// #include <stddef.h>
// #include <stdlib.h>
#include <stdio.h>

#define BAUDRATE 9600
#define BAUD_PRESCALER (((F_CPU / (BAUDRATE * 16UL))) - 1)

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
#define CEH_RQ_DDR DDRB
#define CEH_RQ_PIN PIN4
#define CEH_RQ_PINP PINB
#define CEH_RQ_PORT PORTB
// Led UI (mini-pro pin 13 - onboard led - output)
#define LED_UI_DDR DDRB
#define LED_UI_PIN PIN5
#define LED_UI_PINP PINB
#define LED_UI_PORT PORTB
// DHW potentiometer (mini-pro pin A0 - intput)
#define DHW_POT ADC0
// CH potentiometer (mini-pro pin A1 - intput)
#define CEH_POT ADC1
// System potentiometer (mini-pro pin A2 - intput)
#define SYS_POT ADC2
// Air-flow sensor (mini-pro pin A3 - intput) Board 4-6 left to right
#define AIRFLOW_DDR DDRC
#define AIRFLOW_PIN PIN3
#define AIRFLOW_PINP PINC
#define AIRFLOW_PORT PORTC
// DHW temperature sensor (mini-pro pin A6 - intput)
#define DHW_TEMP ADC6
// CH temperature sensor (mini-pro pin A7 - intput)
#define CEH_TEMP ADC7

// Types
typedef enum state {
    OFF = 0,
    READY = 10,
    IGNITING = 20,
    ON_DHW_DUTY = 30,
    ON_CEH_DUTY = 40,
    ERROR = 100
} State;
typedef enum ignition {
    IG_1 = 21,
    IG_2 = 22,
    IG_3 = 23,
    IG_4 = 24,
    IG_5 = 25,
    IG_OK = 26,
    IG_ERR = 29
} IgnitionState;
typedef enum flag_byte {
    INPUT_FLAGS = 0,
    OUTPUT_FLAGS = 1
} FlagsByte;
typedef enum input_flags {
    DHW_REQUEST = 0,
    CEH_REQUEST = 1,
    AIRFLOW = 2,
    FLAME_SENSOR = 3,
    OVERHEAT = 4
} InputFlag;
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
typedef struct sys_info {
    State system_state;
    uint8_t input_flags;
    uint8_t output_flags;
    uint16_t dhw_temp;
    uint16_t ceh_temp;
    uint16_t dhw_pot;
    uint16_t ceh_pot;
    uint16_t sys_pot;
    char mode_lit[20];
    uint8_t pre_iflags;
    uint8_t pre_oflags;
} SysInfo;

// Prototypes
void SerialInit(void);
unsigned char SerialRxChr(void);
void SerialTxChr(unsigned char);
void SerialTxNum(uint16_t);
void SerialTxStr(char *);
void ClrScr(void);
void Dashboard(SysInfo *);
void DrawLine(uint8_t, char);
void InitLedUi(void);
void SetLedUiOn(void);
void SetLedUiOff(void);
void InitFlags(SysInfo *, FlagsByte);
void SetFlag(SysInfo *, FlagsByte, uint8_t);
void ClearFlag(SysInfo *, FlagsByte, uint8_t);
bool GetFlag(SysInfo *, FlagsByte, uint8_t);
void InitActuator(SysInfo *, OutputFlag);
void ControlActuator(SysInfo *, OutputFlag, HwSwitch);
void InitSensor(SysInfo *, InputFlag);
bool CheckSensor(SysInfo *, InputFlag);

// Globals
const char __flash str_header_01[] = {"| OPEN-BOILER v0.4 (2019-08-10)"};
const char __flash str_header_02[] = {" *** Hi Juan, Sandra & Gustavo! *** "};
const char __flash str_iflags[] = {"Input flags: "};
const char __flash str_oflags[] = {"Output flags: "};
const char __flash str_lit_00[] = {"DHW request: "};
const char __flash str_lit_01[] = {"CH request: "};
const char __flash str_lit_02[] = {"Airflow: "};
const char __flash str_lit_03[] = {"Flame: "};
const char __flash str_lit_04[] = {"Overheat: "};
const char __flash str_lit_05[] = {"Exhaust fan: "};
const char __flash str_lit_06[] = {"Water pump: "};
const char __flash str_lit_07[] = {"Spark igniter: "};
const char __flash str_lit_08[] = {"Security valve: "};
const char __flash str_lit_09[] = {"Valve-1: "};
const char __flash str_lit_10[] = {"Valve-2: "};
const char __flash str_lit_11[] = {"Valve-3: "};
const char __flash str_lit_12[] = {"LED UI: "};
const char __flash str_lit_13[] = {"DHW temp: "};
const char __flash str_lit_14[] = {"CH temp: "};
const char __flash str_lit_15[] = {"DHW pot: "};
const char __flash str_lit_16[] = {"CH pot: "};
const char __flash str_true[] = {"Yes"};
const char __flash str_false[] = {"No "};
const char __flash str_crlf[] = {"\r\n"};
const uint8_t __flash clr_ascii[] = {27, 91, 50, 74, 27, 91, 72};
const char __flash str_mode_00[] = {" [ OFF ]\n\r"};
const char __flash str_mode_10[] = {" [ READY ]\n\r"};
const char __flash str_mode_20[] = {" [ IGNITING ]\n\r"};
const char __flash str_mode_30[] = {" [ ON_DHW_DUTY ]\n\r"};
const char __flash str_mode_40[] = {" [ ON_CH_DUTY ]\n\r"};
const char __flash str_mode_100[] = {" [ ERROR ]\n\r"};

#endif /* VICTORIA_CONTROL_H_ */
