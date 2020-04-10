/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: sys-types.h (system types) for ATmega328
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-10 (Easter quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef _SYS_GLOBALS_H_
#define _SYS_GLOBALS_H_

// This software
#define FW_NAME "OPEN-BOILER"
#define FW_VERSION "v0.7 x3"
#define FW_ALIAS "\"eXperimental Test-06\"    "

//Types
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

typedef enum analog_inputs {
    DHW_TEMPERATURE = 6,
    CH_TEMPERATURE = 7,
    DHW_SETTING = 0,
    CH_SETTING = 1,
    SYSTEM_SETTING = 2
} AnalogInput;

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
    //SystemTimers timer_buffer[3];
} SysInfo;

#endif /* _SYS_GLOBALS_H_ */
