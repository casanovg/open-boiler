/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: victoria-control.c (main code) for ATmega328
 *  ........................................................
 *  Version: 0.8 "Juan" / 2019-04-09 ("Easter quarantine")
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

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

    // Initialize USART for serial communications (38400, N, 8, 1)
    SerialInit();

    // Enable global interrupts
    sei();
    SetTickTimer();

    ClrScr();
    SerialTxStr(str_crlf);
    SerialTxStr(str_header_01);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);

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

    // Electromechanical switches debouncing initialization
    DebounceSw debounce_sw;
    DebounceSw *p_debounce = &debounce_sw;
    p_debounce->airflow_deb = DLY_DEBOUNCE_CH_REQ;
    p_debounce->ch_request_deb = DLY_DEBOUNCE_AIRFLOW;

    // NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW
    GasModulator gas_modulator[] = {
        {VALVE_1, 7000, 0.87, 0},
        {VALVE_2, 12000, 1.46, 0},
        {VALVE_3, 20000, 2.39, 0}};

    static const uint16_t cycle_time = 10000;
    //uint8_t cycle_slots = 6;
    bool cycle_in_progress = 0;
    uint8_t system_valves = (sizeof(gas_modulator) / sizeof(gas_modulator[0]));
    uint8_t current_heat_level = 1; /* This level is determined by the CH temperature potentiometer */
    //uint8_t current_heat_level = GetHeatLevel(p_system->ch_setting, DHW_SETTING_STEPS);
    uint8_t current_valve = 0;
    uint32_t valve_open_timer = 0;

    // Set timer for gas valve modulation
    SetTimer(VALVE_OPEN_TIMER_ID, (unsigned long)VALVE_OPEN_TIMER_DURATION, RUN_ONCE_AND_HOLD);

    // NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW

    // Delay
    uint16_t delay = 0;

    // ADC buffers initialization
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_01);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif /* SERIAL_DEBUG */    
    AdcBuffers buffer_pack;
    AdcBuffers *p_buffer_pack = &buffer_pack;
    InitAdcBuffers(p_buffer_pack, BUFFER_LENGTH);

    // Initialize actuator controls
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_02);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif /* SERIAL_DEBUG */    
    for (OutputFlag device = EXHAUST_FAN; device <= LED_UI; device++) {
        InitActuator(p_system, device);
    }

    // Turn all actuators off
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_03);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif /* SERIAL_DEBUG */
    for (OutputFlag device = EXHAUST_FAN; device <= LED_UI; device++) {
        ClearFlag(p_system, OUTPUT_FLAGS, device);
        _delay_ms(5);
    }

    // Initialize digital sensor flags
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_04);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif /* SERIAL_DEBUG */
    for (InputFlag digital_sensor = DHW_REQUEST; digital_sensor <= OVERHEAT; digital_sensor++) {
        InitDigitalSensor(p_system, digital_sensor);
    }

    // Initialize analog sensor inputs
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_05);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif /* SERIAL_DEBUG */    
    for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
        InitAnalogSensor(p_system, analog_sensor);
    }

    // Pre-load analog sensor values
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_06);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif /* SERIAL_DEBUG */    
    for (uint8_t i = 0; i < BUFFER_LENGTH; i++) {
        for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
            CheckAnalogSensor(p_system, p_buffer_pack, analog_sensor, false);
        }
    }

#if SERIAL_DEBUG
    SerialTxStr(str_preboot_07);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif /* SERIAL_DEBUG */

    _delay_ms(500);

    // Show system operation status
    Dashboard(p_system, false);

    // Add a 2 seconds delay before activating the WDT
    _delay_ms(2000);
    // If the system freezes, reset the microcontroller after 8 seconds
    wdt_enable(WDTO_8S);

    /*  ___________________
       |                   | 
       |     Main Loop     |
       |___________________|
    */
    for (;;) {
        // Reset the WDT
        wdt_reset();

        // Update digital input sensors status
        for (InputFlag digital_sensor = DHW_REQUEST; digital_sensor <= OVERHEAT; digital_sensor++) {
            CheckDigitalSensor(p_system, digital_sensor, p_debounce, false);
        }

        // Update analog input sensors status
        for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
            CheckAnalogSensor(p_system, p_buffer_pack, analog_sensor, false);
        }

        // If the pump is working, check delay counter to turn it off
        if ((p_system->output_flags >> WATER_PUMP) & true) {
            if (!(p_system->pump_delay--)) {
                ClearFlag(p_system, OUTPUT_FLAGS, WATER_PUMP);
                p_system->pump_delay = 0;
            }
        }

        // DHW temperature sensor out of range -> Error 008
        if ((p_system->dhw_temperature <= ADC_MIN) || (p_system->dhw_temperature >= ADC_MAX)) {
            GasOff(p_system);
            p_system->error = ERROR_008;
            p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
        }

        // CH temperature sensor out of range -> Error 009
        if ((p_system->ch_temperature <= ADC_MIN) || (p_system->ch_temperature >= ADC_MAX)) {
            GasOff(p_system);
            p_system->error = ERROR_009;
            p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
        }

        // Unexpected CH overheat -> Error 010
        if (p_system->ch_temperature < (CH_SETPOINT_HIGH - MAX_CH_TEMP_TOLERANCE)) {
            GasOff(p_system);
            p_system->error = ERROR_010;
            p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
        }

#if !(OVERHEAT_OVERRIDE)
        // Verify that the overheat thermostat is not open, otherwise, there's a failure
        if ((p_system->input_flags >> OVERHEAT) & true) {
            p_system->error = ERROR_001;
            p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
        }
#endif /* OVERHEAT_OVERRIDE */

        // Display updated status
        Dashboard(p_system, false);

        // System FSM
        switch (p_system->system_state) {
            /*  ________________________
              |                         |
              |   System state -> OFF   |
              |_________________________|
            */
            case OFF: {
                // Verify that the flame sensor is off at this point, otherwise, there's a failure
                if ((p_system->input_flags >> FLAME) & true) {
                    GasOff(p_system);
                    p_system->error = ERROR_002;
                    p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
                    break;
                }
#if !(AIRFLOW_OVERRIDE)
                // If there isn't a fan test in progress, verify that the airflow sensor is off, otherwise, there's a failure
                if ((p_system->inner_step < OFF_3) && ((p_system->input_flags >> AIRFLOW) & true)) {
                    ClearFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN);
                    p_system->error = ERROR_003;
                    p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
                    break;
                }
#endif /* AIRFLOW_OVERRIDE */
                switch (p_system->inner_step) {
                    // .....................................
                    // . Step OFF_1 : Turn all devices off  .
                    // .....................................
                    case OFF_1: {
                        // Turn all actuators off, except the CH water pump
                        GasOff(p_system);
                        delay = DLY_L_OFF_2;
                        p_system->inner_step = OFF_2;
                        break;
                    }
                    // ..................................................
                    // . Step OFF_2 : Fan test in progress: turn fan on  .
                    // ..................................................
                    case OFF_2: {
#if (!(AIRFLOW_OVERRIDE) && !(FAN_TEST_OVERRIDE))
                        if (!(delay--)) {                                 /* DLY_L_OFF_2 */
                            SetFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN); /* Turn exhaust fan on */
                            delay = DLY_L_OFF_3;
                            p_system->inner_step = OFF_3;
                        }
#else
                        p_system->inner_step = OFF_3;
#endif /* AIRFLOW_OVERRIDE && FAN_TEST_OVERRIDE */
                        break;
                    }
                    // .....................................................................
                    // . Step OFF_3 : Fan test in progress: check airflow sensor activation .
                    // .....................................................................
                    case OFF_3: {
#if (!(AIRFLOW_OVERRIDE) && !(FAN_TEST_OVERRIDE))
                        // Airflow sensor activated -> fan test successful
                        if ((p_system->input_flags >> AIRFLOW) & true) {
                            ClearFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN);
                            delay = DLY_L_OFF_4;
                            p_system->inner_step = OFF_4;
                        }
                        // Timeout: Airflow sensor didn't activate on time -> fan test failed
                        if (!(delay--)) { /* DLY_L_OFF_3 */
                            ClearFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN);
                            p_system->error = ERROR_004;
                            p_system->system_state = ERROR;
                        }
#else
                        // Airflow sensor check skipped
                        p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                        delay = DLY_L_OFF_4;
                        p_system->inner_step = OFF_4;
#endif /* AIRFLOW_OVERRIDE && FAN_TEST_OVERRIDE */
                        break;
                    }
                    // .......................................................................
                    // . Step OFF_4 : Fan test in progress: check airflow sensor deactivation .
                    // .......................................................................
                    case OFF_4: {
                        //LED_UI_PORT |= (1 << LED_UI_PIN);       // @@@@@
                        if (!(delay--)) { /* DLY_L_OFF_4 --- Let the fan to rev down --- */
                                          //LED_UI_PORT &= ~(1 << LED_UI_PIN);  // @@@@@
#if (!(AIRFLOW_OVERRIDE) && !(FAN_TEST_OVERRIDE))
                            if ((p_system->input_flags >> AIRFLOW) & true) {
                                p_system->error = ERROR_006;
                                p_system->system_state = ERROR;
                            } else {
                                p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                                delay = DLY_L_READY_1;
                                p_system->inner_step = READY_1;
                                p_system->system_state = READY;
                            }
#else
                            p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                            delay = DLY_L_READY_1;
                            p_system->inner_step = READY_1;
                            p_system->system_state = READY;
#endif /* AIRFLOW_OVERRIDE && FAN_TEST_OVERRIDE */
                        }
                        break;
                    }
                    // ................
                    // . OFF : Default .
                    // ................
                    default: {
                        if (p_system->system_state == OFF) {
                            p_system->inner_step = OFF_1;
                        }
                        break;
                    }
                }
                break;
            }

            /*  __________________________
              |                           |
              |   System state -> READY   |
              |___________________________|
            */
            case READY: {
                // Give the flame sensor time before checking if it is off when the gas is closed
                if (delay < (DLY_L_READY_1 - DLY_FLAME_OFF)) {
                    // Verify that the flame sensor is off at this point, otherwise, there's a failure
                    if ((p_system->input_flags >> FLAME) & true) {
                        p_system->error = ERROR_002;
                        p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
                    }
                }
                // If the water pump still has time to run before shutting
                // down, let it run until the delay counter reaches zero
                if (p_system->pump_delay > 0) {
                    SetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP);
                }
#if !(AIRFLOW_OVERRIDE)
                // Give the airflow sensor time before checking if it switches off when the fan gets turned off
                if (delay < (DLY_L_READY_1 - DLY_AIRFLOW_OFF)) {
                    // Verify that the airflow sensor is off at this point, otherwise, there's a failure
                    if ((p_system->input_flags >> AIRFLOW) & true) {
                        p_system->error = ERROR_003;
                        p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
                    }
                }
#endif /* AIRFLOW_OVERRIDE */
                // Check if there a DHW or CH request. If both are requested, DHW will have higher priority after ignition
                if (((p_system->input_flags >> DHW_REQUEST) & true) || ((p_system->input_flags >> CH_REQUEST) & true)) {
                    p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                    delay = DLY_L_IGNITING_1;
                    p_system->inner_step = IGNITING_1;
                    p_system->system_state = IGNITING;
                }
                if (!(delay--)) { /* DLY_L_READY_1 */
                    delay = DLY_L_READY_1;
                    p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                }
                break;
            }

            /*  _____________________________
              |                              |
              |   System state -> IGNITING   |
              |______________________________|
            */
            case IGNITING: {
                // Check if the DHW and CH request are over
                if ((((p_system->input_flags >> DHW_REQUEST) & true) == false) &&
                    (((p_system->input_flags >> CH_REQUEST) & true) == false)) {
                    // Request canceled, turn actuators off and return to "ready" state
                    GasOff(p_system);
                    p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                    delay = DLY_L_READY_1;
                    p_system->inner_step = READY_1;
                    p_system->system_state = READY;
                }
                switch (p_system->inner_step) {
                    // .................................
                    // . Step IGNITING_1 : Turn fan on  .
                    // .................................
                    case IGNITING_1: {
                        if (!(delay--)) {                                 /* DLY_L_IGNITING_1 */
                            SetFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN); /* Turn exhaust fan on */
                            delay = DLY_L_IGNITING_2;
                            p_system->inner_step = IGNITING_2;
                        }
                        break;
                    }
                    // ..........................................
                    // . Step IGNITING_2 : Check airflow sensor  .
                    // ..........................................
                    case IGNITING_2: {
#if !(AIRFLOW_OVERRIDE)
                        // Airflow sensor activated -> continue ignition sequence
                        if ((p_system->input_flags >> AIRFLOW) & true) {
                            delay = DLY_L_IGNITING_3;
                            p_system->inner_step = IGNITING_3;
                        }
                        // Airflow sensor activation timeout -> ignition sequence canceled
                        if (!(delay--)) { /* DLY_L_IGNITING_2 */
                            ClearFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN);
                            p_system->error = ERROR_004;
                            p_system->system_state = ERROR;
                        }
#else
                        // Airflow sensor check skipped
                        p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                        delay = DLY_L_IGNITING_3;
                        p_system->inner_step = IGNITING_3;
#endif /* AIRFLOW_OVERRIDE */
                        break;
                    }
                    // .............................................
                    // . Step IGNITING_3 : Open gas security valve  .
                    // .............................................
                    case IGNITING_3: {
                        if (!(delay--)) { /* DLY_L_IGNITING_3 */
                            SetFlag(p_system, OUTPUT_FLAGS, VALVE_S);
                            delay = DLY_L_IGNITING_4;
                            p_system->inner_step = IGNITING_4;
                        }
                        break;
                    }
                    // ......................................................
                    // . Step IGNITING_4 : Open gas valve 1 or 2 alternately .
                    // ......................................................
                    case IGNITING_4: {
                        if (!(delay--)) { /* DLY_L_IGNITING_4 */
                            if (p_system->ignition_retries == 0) {
                                ClearFlag(p_system, OUTPUT_FLAGS, VALVE_2);
                                SetFlag(p_system, OUTPUT_FLAGS, VALVE_1);
                            } else {
                                ClearFlag(p_system, OUTPUT_FLAGS, VALVE_1);
                                SetFlag(p_system, OUTPUT_FLAGS, VALVE_2);
                            }

                            delay = DLY_L_IGNITING_5;
                            p_system->inner_step = IGNITING_5;
                        }
                        break;
                    }
                    // ..........................................
                    // . Step IGNITING_5 : Turn spark igniter on .
                    // ..........................................
                    case IGNITING_5: {
                        if (!(delay--)) { /* DLY_L_IGNITING_5 */
                            SetFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER);
                            // Stretch flame detection timeout on each ignition retry
                            delay = DLY_L_IGNITING_6 * ((p_system->ignition_retries + 1) ^ 2);
                            p_system->inner_step = IGNITING_6;
                        }
                        break;
                    }
                    // ...............................................................................
                    // . Step IGNITING_6 : Check flame and hand over control to the requested service .
                    // ...............................................................................
                    // If the request ceased, close gas and return to "ready" state.
                    // If there is no flame on time, close gas and go to "error" state.
                    case IGNITING_6: {
#if FAST_FLAME_DETECTION
                        // Spark igniter is turned off ass soon as the flame is detected instead
                        // instead of checking the flame sensor after a delay.
                        // NOTE: False positives due to sparks could cause ignition retries
                        if ((p_system->input_flags >> FLAME) & true) {
                            // Turn spark igniter off
                            ClearFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER);
                        }
#endif                                    /* FAST_FLAME_DETECTION */
                        if (!(delay--)) { /* DLY_L_IGNITING_6 */
                            // Increment ignition retry counter
                            p_system->ignition_retries++;
                            // Turn spark igniter off
                            ClearFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER);
                            // If the burner is lit on time
                            if ((p_system->input_flags >> FLAME) & true) {
                                // Turn spark igniter off
                                ClearFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER);
                                // Reset ignition retry counter
                                p_system->ignition_retries = 0;
                                // Hand over control to the requested service (DHW has higher priority)
                                if ((p_system->input_flags >> DHW_REQUEST) & true) {
                                    delay = DLY_L_FLAME_MODULATION / 3;
                                    p_system->inner_step = DHW_ON_DUTY_1;
                                    p_system->system_state = DHW_ON_DUTY;
                                } else if ((p_system->input_flags >> CH_REQUEST) & true) {
                                    delay = DLY_L_CH_ON_DUTY_LOOP;
                                    p_system->inner_step = CH_ON_DUTY_1;
                                    p_system->system_state = CH_ON_DUTY;
                                } else {
                                    // Request canceled, turn actuators off and return to "ready" state
                                    GasOff(p_system);
                                    delay = DLY_L_READY_1;
                                    p_system->inner_step = READY_1;
                                    p_system->system_state = READY;
                                }
                            } else {
                                if (p_system->ignition_retries >= MAX_IGNITION_RETRIES) {
                                    GasOff(p_system);
                                    p_system->ignition_retries = 0;
                                    p_system->error = ERROR_005;
                                    p_system->system_state = ERROR;
                                } else {
                                    delay = DLY_L_IGNITING_4;
                                    p_system->inner_step = IGNITING_4;
                                }
                            }
                        }
                        break;
                    }
                    // .....................
                    // . IGNITING : Default .
                    // .....................
                    default: {
                        if (p_system->system_state == IGNITING) {
                            delay = DLY_L_IGNITING_1;
                            p_system->inner_step = IGNITING_1;
                        }
                        break;
                    }
                }
                break;
            }

            /*  ________________________________
              |                                 |
              |   System state -> DHW_ON_DUTY   |
              |_________________________________|
            */
            case DHW_ON_DUTY: {
                // If the flame sensor is off, check that gas valves 3 and 2 are closed and retry ignition
                if (((p_system->input_flags >> FLAME) & true) == false) {
                    if ((p_system->output_flags >> VALVE_3) & true) {
                        ClearFlag(p_system, OUTPUT_FLAGS, VALVE_3);
                    }
                    if ((p_system->output_flags >> VALVE_2) & true) {
                        ClearFlag(p_system, OUTPUT_FLAGS, VALVE_2);
                    }
                    delay = DLY_L_IGNITING_1;
                    p_system->inner_step = IGNITING_1;
                    p_system->system_state = IGNITING;
                }
#if !(AIRFLOW_OVERRIDE)
                // Verify that the airflow sensor is on, otherwise, close gas and go to error
                if (((p_system->input_flags >> AIRFLOW) & true) == false) {
                    GasOff(p_system); /* Close gas, turn igniter and fan off */
                    p_system->error = ERROR_007;
                    p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
                }
#endif /* AIRFLOW_OVERRIDE */
                // If the pump is on, halt it ...
                if ((p_system->output_flags >> WATER_PUMP) & true) {
                    ClearFlag(p_system, OUTPUT_FLAGS, WATER_PUMP);
                }
                // Check if the DHW request is over
                if (((p_system->input_flags >> DHW_REQUEST) & true) == false) {
                    // If there is a CH request active, modulate to valve 1 and
                    // hand over control to CH service state
                    if ((p_system->input_flags >> CH_REQUEST) & true) {
                        p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                        delay = DLY_L_CH_ON_DUTY_LOOP;
                        p_system->inner_step = p_system->ch_on_duty_step;
                        p_system->system_state = CH_ON_DUTY;
                    } else {
                        // DHW request canceled, turn gas off and return to "ready" state
                        GasOff(p_system);
                        delay = DLY_L_READY_1;
                        p_system->inner_step = READY_1;
                        p_system->system_state = READY;
                    }
                }

                //
                // [ # # # ] DHW heat modulation code  [ # # # ]
                //

                if (cycle_in_progress == false) {
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
#else
                            // Read the DHW potentiometer to determine current heat level
                            current_heat_level = GetHeatLevel(p_system->ch_setting, DHW_SETTING_STEPS);
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

                //
                // [ # # # ] DHW heat modulation code end [ # # # ]
                //
            }

            /*  _______________________________
              |                                |
              |   System state -> CH_ON_DUTY   |
              |________________________________|
            */
            case CH_ON_DUTY: {
                switch (p_system->inner_step) {
                    // ............................................
                    // . Step CH_ON_DUTY_1 : CH heating, burner on .
                    // ............................................
                    case CH_ON_DUTY_1: {
                        // If the flame sensor is off, check that gas valves 3 and 2 are closed and retry ignition
                        if (((p_system->input_flags >> FLAME) & true) == false) {
                            if ((p_system->output_flags >> VALVE_3) & true) {
                                ClearFlag(p_system, OUTPUT_FLAGS, VALVE_3);
                            }
                            if ((p_system->output_flags >> VALVE_2) & true) {
                                ClearFlag(p_system, OUTPUT_FLAGS, VALVE_2);
                            }
                            delay = DLY_L_IGNITING_1;
                            p_system->inner_step = IGNITING_1;
                            p_system->system_state = IGNITING;
                        }
#if !(AIRFLOW_OVERRIDE)
                        // Verify that the airflow sensor is on, otherwise, close gas and go to error
                        if (((p_system->input_flags >> AIRFLOW) & true) == false) {
                            GasOff(p_system); /* Close gas, turn igniter and fan off */
                            p_system->error = ERROR_007;
                            p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
                        }
#endif /* AIRFLOW_OVERRIDE */
                        // Turn CH water pump on
                        if (((p_system->output_flags >> WATER_PUMP) & true) == false) {
                            SetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP);
                        }
                        // Restart continuously the water pump shutdown timeout counter
                        p_system->pump_delay = DLY_WATER_PUMP_OFF;

                        // If there is a DHW request active, modulate and hand over control to DHW service
                        if ((p_system->input_flags >> DHW_REQUEST) & true) {
                            p_system->ch_on_duty_step = CH_ON_DUTY_1; /* Preserve current CH service step */
                            // Close valve 2
                            //ClearFlag(p_system, OUTPUT_FLAGS, VALVE_2);
                            // Open valve 1
                            //SetFlag(p_system, OUTPUT_FLAGS, VALVE_1);
                            //p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                            delay = DLY_L_FLAME_MODULATION / 3;
                            p_system->inner_step = DHW_ON_DUTY_1;
                            p_system->system_state = DHW_ON_DUTY;
                        } else {
                            // Check if the CH request is over
                            if (((p_system->input_flags >> CH_REQUEST) & true) == false) {
                                // CH request canceled, turn gas off and return to "ready" state
                                GasOff(p_system);
                                delay = DLY_L_READY_1;
                                p_system->inner_step = READY_1;
                                p_system->system_state = READY;
                            }
                        }
                        //
                        // [ # # # ] CH heat modulation code [ # # # ]
                        //
                        // While the CH water temperature is cooler than setpoint high, continue heating
                        // Otherwise, close gas and move on to CH_ON_DUTY_2 step
                        // NOTE: The temperature reading last bit is masked out to avoid oscillations
                        if ((p_system->ch_temperature & CH_TEMP_MASK) >= CH_SETPOINT_HIGH) {
                            if ((p_system->output_flags >> VALVE_1) & true) {
                                // Close valve 1
                                ClearFlag(p_system, OUTPUT_FLAGS, VALVE_1);
                                // Open valve 2
                                SetFlag(p_system, OUTPUT_FLAGS, VALVE_2);
                            }
                        } else {
                            //Close gas
                            GasOff(p_system);
                            // Restart the water pump shutdown timeout counter
                            //p_system->pump_delay = DLY_WATER_PUMP_OFF;
                            p_system->inner_step = CH_ON_DUTY_2;
                        }
                        //
                        // [ # # # ] CH heat modulation code end [ # # # ]
                        //
                        break;
                    }
                    // .........................................................
                    // . Step CH_ON_DUTY_2 : CH recirculating water, burner off .
                    // .........................................................
                    case CH_ON_DUTY_2: {
                        // Close gas
                        if ((p_system->input_flags >> FLAME) & true) {
                            GasOff(p_system);
                        }
                        // Turn CH water pump on
                        if (((p_system->output_flags >> WATER_PUMP) & true) == false) {
                            SetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP);
                        }
                        // If the CH water temperature is colder than setpoint low, ignite the burner,
                        // then go back to CH_ON_DUTY_1 step
                        // NOTE: The temperature reading last bit is masked out to avoid oscillations
                        if ((p_system->ch_temperature & CH_TEMP_MASK) >= CH_SETPOINT_LOW) {
                            p_system->inner_step = CH_ON_DUTY_1;
                            delay = DLY_L_IGNITING_1;
                            p_system->inner_step = IGNITING_1;
                            p_system->system_state = IGNITING;
                            break;
                        }
                        // If there is a DHW request active, ignite and hand over control to DHW service
                        if ((p_system->input_flags >> DHW_REQUEST) & true) {
                            p_system->ch_on_duty_step = CH_ON_DUTY_2; /* Preserve current CH service step */
                            p_system->last_displayed_iflags = 0xFF;   /* Force a display dashboard refresh */
                            delay = DLY_L_IGNITING_1;
                            p_system->inner_step = IGNITING_1;
                            p_system->system_state = IGNITING;
                        } else {
                            // Check if the CH request is over
                            if (((p_system->input_flags >> CH_REQUEST) & true) == false) {
                                // CH request canceled, turn gas off and return to "ready" state
                                GasOff(p_system);
                                delay = DLY_L_READY_1;
                                p_system->inner_step = READY_1;
                                p_system->system_state = READY;
                            }
                        }
                        break;
                    }
                    // .......................
                    // . CH_ON_DUTY : Default .
                    // .......................
                    default: {
                        if (p_system->system_state == CH_ON_DUTY) {
                            delay = DLY_L_CH_ON_DUTY_LOOP;
                            p_system->inner_step = p_system->ch_on_duty_step;
                        }
                        break;
                    }
                }
                if (!(delay--)) { /* DLY_L_CH_ON_DUTY_1 */
                    delay = DLY_L_CH_ON_DUTY_LOOP;
                    p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                }
                break;
            }

            /*  __________________________
              |                           |
              |   System state -> ERROR   |
              |___________________________|
            */
            case ERROR: {
                // Turn all actuators off, except the CH water pump
                GasOff(p_system);
                uint8_t error_loops = 5; /* Number of times the error will be displayed */
                // Error loop -> displays the error code "error_loops" times
                while (error_loops--) {
                    // Update digital input sensors status
                    for (InputFlag digital_sensor = DHW_REQUEST; digital_sensor <= OVERHEAT; digital_sensor++) {
                        CheckDigitalSensor(p_system, digital_sensor, p_debounce, false);
                    }
                    p_system->system_state = ERROR;
                    Dashboard(p_system, true);
                    SetFlag(p_system, OUTPUT_FLAGS, LED_UI);
                    //ControlActuator(p_system, LED_UI, TURN_ON, false); /* true updates display on each pass */
                    SerialTxStr(str_crlf);
                    SerialTxStr(str_error_s);
                    SerialTxNum(p_system->error, DIGITS_3);
                    SerialTxStr(str_error_e);
                    SerialTxStr(str_crlf);
                    _delay_ms(500);
                    ClearFlag(p_system, OUTPUT_FLAGS, LED_UI);
                    //ControlActuator(p_system, LED_UI, TURN_OFF, false);
                    _delay_ms(500);
                }
                // End of error loop
                // Next state -> OFF (reset error and try to resume service)
                p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                p_system->error = ERROR_000;
                p_system->inner_step = OFF_1;
                p_system->system_state = OFF;
                break;
            }

            /*  ____________________________
              |                             |
              |   System state -> Default   |
              |_____________________________|
            */
            default: {
                delay = DLY_L_OFF_2;
                p_system->inner_step = OFF_1;
                p_system->system_state = OFF;
                break;
            }

        } /* System FSM end */

    } /* Main loop end */

    return 0;
}

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
