// ---------------------------------------------
// Test 04 - 2020-04-04 - Gustavo Casanova
// .............................................
// Heat modulation algorithm - new
// ---------------------------------------------

#include "victoria-control.h"

// Main function
int main(void) {
    /*  ___________________
       |                   | 
       |    Setup Block    |
       |___________________|
    */

    // Disable watch dog timer
    wdt_disable();

    //Timer setup
    //asm ("sei \n");

    // Enable global interrupts
    sei();
    SetTickTimer();

#define SERIAL_DEBUG true
#define LED_DEBUG true
#define HEAT_MODULATOR_DEMO true

#if SERIAL_DEBUG
    // Initialize USART for serial communications (38400, N, 8, 1)
    SerialInit();

    ClrScr();
    SerialTxStr(str_crlf);
    SerialTxStr(str_header_01);
    SerialTxStr(str_header_02);
    SerialTxStr(str_crlf);
    DrawLine(44, 46);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif
    //System state initialization
    SysInfo sys_info;
    SysInfo *p_system = &sys_info;
    p_system->system_state = OFF;
    p_system->inner_step = OFF_1;
    p_system->input_flags = 0;
    p_system->output_flags = 0;
    p_system->last_displayed_iflags = 0;
    p_system->last_displayed_oflags = 0;
    p_system->error = ERROR_000;
    p_system->ignition_retries = 0;
    p_system->pump_delay = 0;
    p_system->ch_on_duty_step = CH_ON_DUTY_1;
    p_system->dhw_heat_level = 0; /* This level is determined by the CH temperature potentiometer */

    // Electromechanical switches debouncing initialization
    DebounceSw debounce_sw;
    DebounceSw *p_debounce = &debounce_sw;
    p_debounce->airflow_deb = DLY_DEBOUNCE_CH_REQ;
    p_debounce->ch_request_deb = DLY_DEBOUNCE_AIRFLOW;

    // NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW
    // Gas modulator properties
    GasModulator gas_modulator[] = {
        {VALVE_1, 7000, 0.87, 0},
        {VALVE_2, 12000, 1.46, 0},
        {VALVE_3, 20000, 2.39, 0}};

    // Gas modulator variables
    uint8_t current_heat_level = 0; /* The heat level is determined by the CH temperature potentiometer */
    //static const unsigned long heat_cycle_time = 5000;
    uint8_t system_valves = (sizeof(gas_modulator) / sizeof(gas_modulator[0]));
    uint8_t current_valve = 0;
    bool cycle_in_progress = false;
    // NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW

    // Initialize all available system timers
    for (int i = 0; i < TIMER_BUFFER_SIZE; i++) {
        timer_buffer[i].timer_id = TIMER_EMPTY;
        timer_buffer[i].timer_start_time = 0;
        timer_buffer[i].timer_time_lapse = 0;
    }

    // ADC buffers initialization
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_01);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif
    AdcBuffers buffer_pack;
    AdcBuffers *p_buffer_pack = &buffer_pack;
    InitAdcBuffers(p_buffer_pack, BUFFER_LENGTH);

    // Initialize actuator controls
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_02);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif
    for (OutputFlag device = EXHAUST_FAN; device <= LED_UI; device++) {
        InitActuator(p_system, device);
    }

    // Turn all actuators off
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_03);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif
    for (OutputFlag device = EXHAUST_FAN; device <= LED_UI; device++) {
        ClearFlag(p_system, OUTPUT_FLAGS, device);
        _delay_ms(5);
    }

    // Initialize digital sensor flags
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_04);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif
    for (InputFlag digital_sensor = DHW_REQUEST; digital_sensor <= OVERHEAT; digital_sensor++) {
        InitDigitalSensor(p_system, digital_sensor);
    }

    // Initialize analog sensor inputs
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_05);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif
    for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
        InitAnalogSensor(p_system, analog_sensor);
    }

    // Pre-load analog sensor values
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_06);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif
    for (uint8_t i = 0; i < BUFFER_LENGTH; i++) {
        for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
            CheckAnalogSensor(p_system, p_buffer_pack, analog_sensor, false);
        }
    }

    // Show system operation status
    //Dashboard(p_system, false);

    // Add a 2 seconds delay before activating the WDT
    _delay_ms(2000);
    // If the system freezes, reset the microcontroller after 8 seconds
    //************************************************************************wdt_enable(WDTO_8S);

    // Starting FSM cycle
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_07);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif

    /*  ___________________
       |                   | 
       |     Main Loop     |
       |___________________|
    */

    // #############################
    // #
    // # Force activating watch dog
    // #............................
    // # ONLY FOR BOOTLOADER TESTS!
    // #############################
    //for(;;) {};

    // Set timer for gas valve modulation
    SetTimer(VALVE_OPEN_TIMER_ID, (unsigned long)VALVE_OPEN_TIMER_DURATION, RUN_ONCE_AND_HOLD);

    for (;;) {
        // {}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}
        // Valve-toggling heat modulation cycle START
        // ............................................
        if (cycle_in_progress == false) {
            //printf("\n\r============== >>> Cycle start: Heat level %d = %d Kcal/h... <<< ==============\n\r", current_heat_level + 1, heat_level[current_heat_level].kcal_h);
            uint8_t heat_level_time_usage = 0;
#if SERIAL_DEBUG
            //SerialTxStr(str_crlf);
            SerialTxStr(str_heat_mod_01);
            SerialTxNum(current_heat_level, DIGITS_2);
#endif
            // Check heat level integrity
            for (uint8_t vt_check = 0; vt_check < system_valves; vt_check++) {
                heat_level_time_usage += heat_level[current_heat_level].valve_open_time[vt_check];
            }
            if (heat_level_time_usage != 100) {
#if LED_DEBUG
                SetFlag(p_system, OUTPUT_FLAGS, VALVE_S);  // Heat level setting error, the sum of the opening time of all valves must be 100!
                _delay_ms(5000);
                ClearFlag(p_system, OUTPUT_FLAGS, VALVE_S);
#endif
#if SERIAL_DEBUG
                SerialTxStr(str_heat_mod_03);
#endif
                // FAIL-SAFE: One level auto cool down in case of heat cycle error
                current_heat_level--;
                cycle_in_progress = false;
                //return 1;
            } else {
                // Set cycle in progress
#if SERIAL_DEBUG
                SerialTxStr(str_heat_mod_02);
#endif
                cycle_in_progress = true;
                current_valve = 0;
                ResetTimerLapse(VALVE_OPEN_TIMER_ID, (unsigned long)(heat_level[current_heat_level].valve_open_time[current_valve] * HEAT_CYCLE_TIME / 100));
            }
        } else {
            if (TimerFinished(VALVE_OPEN_TIMER_ID)) {
                // Prepare timing for next valve
                current_valve++;
                ResetTimerLapse(VALVE_OPEN_TIMER_ID, (heat_level[current_heat_level].valve_open_time[current_valve] * HEAT_CYCLE_TIME / 100));
                if (current_valve >= system_valves) {
                    // Cycle end: Reset to first valve
#if LED_DEBUG
                    SetFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER);  // Heat level setting error, the sum of the opening time of all valves must be 100!
                    _delay_ms(50);
                    ClearFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER);
#endif
#if SERIAL_DEBUG
                    SerialTxStr(str_crlf);
#endif
                    cycle_in_progress = false;
#if HEAT_MODULATOR_DEMO
                    // DEMO MODE: Heat cycle loop ...
                    if (current_heat_level++ >= (sizeof(heat_level) / sizeof(heat_level[0])) - 1) {
                        current_heat_level = 0;
                    }
#endif
                }
            } else {
                // Turn all heat valves off except the current valve
                for (uint8_t valve_to_close = 0; valve_to_close < system_valves; valve_to_close++) {
                    if (valve_to_close != current_valve) {
                        if (GetFlag(p_system, OUTPUT_FLAGS, gas_modulator[valve_to_close].valve_number)) {
                            //printf(" (X) Closing valve %d ...\n\r", valve_to_close + 1);
                            ClearFlag(p_system, OUTPUT_FLAGS, gas_modulator[valve_to_close].valve_number);
#if SERIAL_DEBUG
                            SerialTxStr(str_heat_mod_06);
                            SerialTxStr(str_heat_mod_04);
                            SerialTxNum(valve_to_close + 1, DIGITS_1);
#endif
                        }
                    }
                }

                // Turn current valve on
                if (GetFlag(p_system, OUTPUT_FLAGS, gas_modulator[current_valve].valve_number) == false) {
                    SetFlag(p_system, OUTPUT_FLAGS, gas_modulator[current_valve].valve_number);
#if SERIAL_DEBUG
                    SerialTxStr(str_heat_mod_05);
                    SerialTxStr(str_heat_mod_04);
                    SerialTxNum(current_valve + 1, DIGITS_1);
                    SerialTxChr(32);
                    SerialTxNum((heat_level[current_heat_level].valve_open_time[current_valve] * HEAT_CYCLE_TIME / 100), DIGITS_FREE);
                    SerialTxStr(str_heat_mod_08);
#endif
                }
            }
        }
        // ............................................
        // Valve-toggling heat modulation cycle END
        // {}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}

    } /* Main loop end */

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////       MAIN FUNCTION END       ////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

// Function SystemRestart
void SystemRestart(void) {
    wdt_enable(WDTO_15MS); /* Restart by activating the watchdog timer */
    for (;;) {
    };
}

// Function InitFlags
void InitFlags(SysInfo *p_sys, FlagsType flags_type) {
    switch (flags_type) {
        case INPUT_FLAGS: {
            p_sys->input_flags = 0;
            break;
        }
        case OUTPUT_FLAGS: {
            p_sys->output_flags = 0;
            break;
        }
        default: {
            break;
        }
    }
}

// Function SetFlag
void SetFlag(SysInfo *p_sys, FlagsType flags_type, uint8_t flag_position) {
    switch (flags_type) {
        case INPUT_FLAGS: {
            p_sys->input_flags |= (1 << flag_position);
            break;
        }
        case OUTPUT_FLAGS: {
            // WARNING !!! HARDWARE ACTIVATION !!!
            p_sys->output_flags |= (1 << flag_position);
            ControlActuator(p_sys, flag_position, TURN_ON, false);
            break;
        }
        default: {
            break;
        }
    }
}

// Function ClearFlag
void ClearFlag(SysInfo *p_sys, FlagsType flags_type, uint8_t flag_position) {
    switch (flags_type) {
        case INPUT_FLAGS: {
            p_sys->input_flags &= ~(1 << flag_position);
            break;
        }
        case OUTPUT_FLAGS: {
            // WARNING !!! HARDWARE DEACTIVATION !!!
            p_sys->output_flags &= ~(1 << flag_position);
            ControlActuator(p_sys, flag_position, TURN_OFF, false);
            break;
        }
        default: {
            break;
        }
    }
}

// Function GetFlag
bool GetFlag(SysInfo *p_sys, FlagsType flags_type, uint8_t flag_position) {
    bool flag;
    switch (flags_type) {
        case INPUT_FLAGS: {
            flag = ((p_sys->input_flags >> flag_position) & true);
            break;
        }
        case OUTPUT_FLAGS: {
            flag = ((p_sys->output_flags >> flag_position) & true);
            break;
        }
    }
    return flag;
}
