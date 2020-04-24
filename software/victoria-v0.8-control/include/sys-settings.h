/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: sys-types.h (system types) for ATmega328
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-10 (Easter Quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef _SYS_SETTINGS_H_
#define _SYS_SETTINGS_H_

#include "hw-mapping.h"

// This software
#define FW_NAME "OPEN-BOILER"
#define FW_VERSION "v0.8"
#define FW_ALIAS "\"Easter Quarantine\"      "

// System defines

#define CH_SETPOINT_HIGH 241  // ADC-NTC CH temperature ~ 55°C
#define CH_SETPOINT_LOW 379   // ADC-NTC CH temperature ~ 38°C

#define DHW_HEAT_CYCLE_TIME 15000  // DHW heat modulator cycle time (milliseconds)
#define CH_HEAT_CYCLE_TIME 20000   // CH heat modulator cycle time (milliseconds)

#define MAX_IGNITION_TRIES 3  // Number of ignition retries when no flame is detected

#define DHW_SETTING_STEPS 12  // DHW setting potentiometer steps
#define CH_SETTING_STEPS 12   // CH setting potentiometer steps
#define SYSTEM_MODE_STEPS 4   // System mode potentiometer steps

#define SYSTEM_TIMERS 6          // Number of system timers
#define HEAT_MODULATOR_VALVES 3  // Number of heat modulator valves

#define OVERHEAT_OVERRIDE false    // True: Overheating thermostat override
#define AIRFLOW_OVERRIDE true      // True: Flue airflow sensor override
#define FAN_TEST_OVERRIDE true     // True: Flue airflow sensor override
#define LED_UI_FOR_FLAME true      // True: Activates onboard LED when the flame detector is on
#define SHOW_DASHBOARD true        // True: Displays the system dashboard on a serial terminal
#define SERIAL_DEBUG false         // True: Shows current heat level and valve timing instead of the dashboard
#define LED_DEBUG false            // True: ONLY FOR DEBUG!!! Toggles SPARK_IGNITER_F on each heat-cycle start and keeps it on to show cycle's valve-time errors
#define HEAT_MODULATOR_DEMO false  // True: ONLY FOR DEBUG!!! loops through all heat levels, from lower to higher. False: NORMAL OPERATION -> Heat modulator code reads DHW potentiometer to determine current heat level
#define TIMER_INDEX_OVF_STOP true  // True: halt system if the system doesn't have enough timer slots (index overflow)!
#define AUTO_DHW_DSP_REFRESH true  // True: Force a dashboard refresh when in a DHW_ON_DUTY loop every DLY_DHW_ON_DUTY_LOOP ms
#define AUTO_CH_DSP_REFRESH true   // True: Force a dashboard refresh when in a CH_ON_DUTY loop every DLY_CH_ON_DUTY_LOOP ms

#define FSM_TIMER_ID 1                    // Main finite state machine timer id
#define FSM_TIMER_DURATION 0              // Main finite state machine timer time-lapse (milliseconds)
#define FSM_TIMER_MODE RUN_ONCE_AND_HOLD  // Main finite state machine timer mode

#define HEAT_TIMER_ID 2                    // Gas modulator heat level timer id
#define HEAT_TIMER_DURATION 0              // Gas modulator heat level timer time-lapse (milliseconds)
#define HEAT_TIMER_MODE RUN_ONCE_AND_HOLD  // Gas modulator heat level timer mode

// This timer controls the water pump auto-shutdown when there are no CH requests
//  * Time: 300000 / 60 / 1000 = 5 min aprox
//  * Time: 600000 / 60 / 1000 = 10 min aprox <- default
//  * Time: 900000 / 60 / 1000 = 15 min aprox
//  * Time: 1800000 / 60 / 1000 = 30 min aprox
#define PUMP_TIMER_ID 3                    // Water pump timer id
#define PUMP_TIMER_DURATION 600000         // Water pump timer time-lapse (milliseconds)
#define PUMP_TIMER_MODE RUN_ONCE_AND_HOLD  // Water pump timer mode

#define DEB_FLAME_TIMER_ID 4                    // Flame detector debounce timer id
#define DEB_FLAME_TIMER_DURATION 500            // Flame detector debounce timer time-lapse
#define DEB_FLAME_TIMER_MODE RUN_ONCE_AND_HOLD  // Flame detector debounce timer mode

#define DEB_CH_SWITCH_TIMER_ID 5                    // Central heating thermostat switch debounce timer id
#define DEB_CH_SWITCH_TIMER_DURATION 250            // Central heating thermostat switch debounce timer time-lapse
#define DEB_CH_SWITCH_TIMER_MODE RUN_ONCE_AND_HOLD  // Central heating thermostat switch debounce timer mode

#define DEB_AIRFLOW_TIMER_ID 6                    // Airflow sensor switch debounce timer id
#define DEB_AIRFLOW_TIMER_DURATION 125            // Airflow sensor switch debounce timer time-lapse
#define DEB_AIRFLOW_TIMER_MODE RUN_ONCE_AND_HOLD  // Airflow sensor switch debounce timer mode

// FSM non-blocking delay times (milliseconds)
#define DLY_OFF_2 10                                      // Off_2: Time before turning the fan for the flue exhaust test
#define DLY_OFF_3 5000                                    // Off_3: Time to let the fan to rev up and the airflow sensor closes (fan test)
#define DLY_OFF_4 3000                                    // Off_4: Time to let the fan to rev down after testing it (when the test is enabled)
#define DLY_READY_1 3500                                  // Ready_1: Time to allow the flame and airflow sensors to switch off after the gas is closed
#define DLY_IGNITING_1 10                                 // Igniting_1: Time-lapse from an ignition request until turning the fan on
#define DLY_IGNITING_2 5000                               // Igniting_2: Time to let the fan to rev up and the airflow sensor closes (ignition)
#define DLY_IGNITING_3 250                                // Igniting_3: Time before opening the security valve after the fan is running
#define DLY_IGNITING_4 125                                // Igniting_4: Time before opening the valve 1 (or 2) after opening the security valve
#define DLY_IGNITING_5 25                                 // Igniting_5: Time before turning the spark igniter on while the valve 1 (or 2) is open
#define DLY_IGNITING_6 (DEB_FLAME_TIMER_DURATION + 2500)  // Igniting_6: Waiting time for flame lit with gas open and spark igniter on before retrying

#if SHOW_DASHBOARD
#define DLY_DHW_ON_DUTY_LOOP 3000  // DHW_on_Duty: Dashboard refreshing time when looping through DHW on-duty mode
#define DLY_CH_ON_DUTY_LOOP 3000   // CH_on_Duty: Dashboard refreshing time when looping through CH on-duty mode
#endif

// System types

typedef enum system_modes {
    SYS_COMBI = 0,
    SYS_DHW = 1,
    SYS_OFF = 2,
    SYS_RESET = 3
} SystemMode;

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
    DHW_TEMPERATURE = DHW_TEMP_ADC,  // ADC6
    CH_TEMPERATURE = CH_TEMP_ADC,    // ADC7
    DHW_SETTING = DHW_POT_ADC,       // ADC0
    CH_SETTING = CH_SET_ADC,         // ADC1
    SYSTEM_MODE = SYS_MOD_ADC        // ADC2
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
    VALVE_1 = 0,
    VALVE_2 = 1,
    VALVE_3 = 2
} HeatValve;

typedef struct heat_level {
    uint8_t valve_open_time[HEAT_MODULATOR_VALVES];
    uint16_t kcal_h;
    float gas_usage;
} HeatLevel;

typedef struct heat_modulator {
    HeatValve heat_valve;  // Heat valve ID
    InputFlag valve_flag;  // Valve flag id number
    uint16_t kcal_h;       // Kcal per hour
    float gas_usage;       // Gas usage per hour
    bool status;           // Valve status
} HeatModulator;

typedef struct sys_info {
    State system_state;                                   // System FSM running state
    InnerStep inner_step;                                 // System FSM state inner step (sub-states)
    uint8_t input_flags;                                  // Flags signaling digital input sensor status
    uint8_t output_flags;                                 // Flags signaling hardware activation status
    uint16_t dhw_temperature;                             // DHW NTC thermistor temperature ADC readout
    uint16_t ch_temperature;                              // CH NTC thermistor temperature ADC readout
    uint16_t dhw_setting;                                 // DWH setting potentiometer ADC readout
    uint16_t ch_setting;                                  // CH setting potentiometer ADC readout
    uint16_t system_mode;                                 // System mode potentiometer ADC readout
    uint8_t last_displayed_iflags;                        // Input sensor flags last shown status
    uint8_t last_displayed_oflags;                        // Hardware activation flags last shown status
    uint8_t ignition_tries;                               // Burner ignition attempts counter
    uint8_t error;                                        // System error code
    uint32_t pump_timer_memory;                           // CH water pump auto-shutdown timer memory
    bool ch_water_overheat;                               // Unexpected central heating water overtemperature flag
    InnerStep ch_on_duty_step;                            // CH inner step before handing over control to DHW
    uint8_t current_heat_level;                           // Current gas modulator heat level, set by the DHW or CH temperature potentiometers
    HeatValve current_valve;                              // Heat modulator current valve
    bool cycle_in_progress;                               // Heat-modulator's heat-level cycle-in-progress flag
    HeatModulator heat_modulator[HEAT_MODULATOR_VALVES];  // Heat modulator struct
} SysInfo;

#endif  // _SYS_SETTINGS_H_
