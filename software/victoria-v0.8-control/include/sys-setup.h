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

#ifndef _SYS_SETUP_H_
#define _SYS_SETUP_H_

// This software
#define FW_NAME "OPEN-BOILER"
#define FW_VERSION "v0.8a"
#define FW_ALIAS "\"Easter Quarantine\"       "

// System defines

#define CH_SETPOINT_HIGH 241 /* ADC-NTC CH temperature ~ 55°C */
#define CH_SETPOINT_LOW 379  /* ADC-NTC CH temperature ~ 38°C */

#define HEAT_CYCLE_TIME 10000 /* Heat modulator cycle time (milliseconds) */

#define MAX_IGNITION_RETRIES 3 /* Number of ignition retries when no flame is detected */

#define DHW_SETTING_STEPS 12 /* DHW setting potentiometer steps */
#define CH_SETTING_STEPS 12  /* CH setting potentiometer steps */

#define GAS_MODULATOR_VALVES 3 /* Number of gas modulator valves */

#define FSM_TIMER_ID 1                   /* Main finite state machine timer id */
#define FSM_TIMER_DURATION 0             /* Main finite state machine timer time-lapse */
#define FSM_TIMER_MODE RUN_ONCE_AND_HOLD /* Main finite state machine timer mode */

#define PUMP_TIMER_ID 2                   /* Water pump timer id */
#define PUMP_TIMER_DURATION 0             /* Water pump timer time-lapse */
#define PUMP_TIMER_MODE RUN_ONCE_AND_HOLD /* Water pump timer mode */

#define OVERHEAT_OVERRIDE false /* True: Overheating thermostat override */

#define AIRFLOW_OVERRIDE true /* True: Flue airflow sensor override */

#define FAN_TEST_OVERRIDE true /* True: Flue airflow sensor override */

#define FAST_FLAME_DETECTION false /* True: Spark igniter is turned off when the flame is detected */
                                   /*       instead of checking the flame sensor after a delay     */

#define LED_UI_FOR_FLAME true /* True: Activates onboard LED when the flame detector is on */

#define HEAT_MODULATOR_DEMO false /* True: ONLY FOR DEBUG!!! loops through all heat levels, from lower to higher */
                                  /* False: Heat modulator code reads DHW potentiometer to determine current level */

#define SERIAL_DEBUG true /* True: Shows current heat level and valve timing instead of the dashboard */

#define LED_DEBUG false /* True: ONLY FOR DEBUG!!! Toggles SPARK_IGNITER_F on each heat-cycle start and keeps it on to show cycle's valve-time errors */

// System types

typedef enum flags_types {
    INPUT_FLAGS = 0,
    OUTPUT_FLAGS = 1
} FlagsType;

typedef enum input_flags {
    DHW_REQUEST_F = 0,
    CH_REQUEST_F = 1,
    AIRFLOW_F = 2,
    FLAME_F = 3,
    OVERHEAT_F = 4
} InputFlag;

typedef enum output_flags {
    EXHAUST_FAN_F = 0,
    WATER_PUMP_F = 1,
    SPARK_IGNITER_F = 2,
    VALVE_S_F = 3,
    VALVE_1_F = 4,
    VALVE_2_F = 5,
    VALVE_3_F = 6,
    LED_UI_F = 7
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

typedef enum heat_valves {
    VALVE_1 = 1,
    VALVE_2 = 2,
    VALVE_3 = 3
} HeatValve;

typedef struct heat_level {
    uint8_t valve_open_time[GAS_MODULATOR_VALVES];
    uint16_t kcal_h;
    float gas_usage;
} HeatLevel;

typedef struct gas_modulator {
    HeatValve heat_valve; /* Heat valve ID */
    InputFlag valve_flag; /* Valve flag id number */
    uint16_t kcal_h;      /* Kcal per hour */
    float gas_usage;      /* Gas usage per hour */
    bool status;          /* Valve status */
} GasModulator;

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
    uint32_t pump_delay;                              /* CH water pump auto-shutdown timer */
    InnerStep ch_on_duty_step;                        /* CH inner step before handing over control to DHW */
    uint8_t dhw_heat_level;                           /* DHW current heat level */
    uint8_t ch_heat_level;                            /* DHW current heat level */
    GasModulator gas_modulator[GAS_MODULATOR_VALVES]; /* Gas Modulator */
    //SystemTimers timer_buffer[3];
} SysInfo;

#endif /* _SYS_SETUP_H_ */
