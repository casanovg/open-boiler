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

    // Initialize USART for serial communications (57600, N, 8, 1)
    SerialInit();

    ClrScr();
    SerialTxStr(str_crlf);
    SerialTxStr(str_header_01);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);

    // System gas modulator
    GasModulator gas_modulator[] = {
        {VALVE_1, VALVE_1_F, 7000, 0.87, false},
        {VALVE_2, VALVE_2_F, 12000, 1.46, false},
        {VALVE_3, VALVE_3_F, 20000, 2.39, false}};

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
    for (uint8_t valve = 0; valve < GAS_MODULATOR_VALVES; valve++) {
        p_system->gas_modulator[valve] = gas_modulator[valve];
    }

    // Electromechanical switches debouncing initialization
    DebounceSw debounce_sw;
    DebounceSw *p_debounce = &debounce_sw;
    p_debounce->airflow_deb = DLY_DEBOUNCE_CH_REQ;
    p_debounce->ch_request_deb = DLY_DEBOUNCE_AIRFLOW;

    // NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW

    //static const uint16_t cycle_time = 10000;
    //uint8_t cycle_slots = 6;
    bool cycle_in_progress = 0;
    //uint8_t system_valves = (sizeof(gas_modulator) / sizeof(gas_modulator[0]));
    uint8_t current_heat_level = 1; /* This level is determined by the CH temperature potentiometer */
    //uint8_t current_heat_level = GetHeatLevel(p_system->ch_setting, DHW_SETTING_STEPS);
    uint8_t current_valve = 0;
    //uint32_t valve_open_timer = 0;

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

    // Initialize system flag byes
    InitFlags(p_system, INPUT_FLAGS);
    InitFlags(p_system, OUTPUT_FLAGS);

    // Initialize actuator controls
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_02);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif /* SERIAL_DEBUG */
    for (OutputFlag device = EXHAUST_FAN_F; device <= LED_UI_F; device++) {
        InitActuator(p_system, device);
    }

    // Turn all actuators off
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_03);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif /* SERIAL_DEBUG */
    for (OutputFlag device = EXHAUST_FAN_F; device <= LED_UI_F; device++) {
        ClearFlag(p_system, OUTPUT_FLAGS, device);
        _delay_ms(5);
    }

    // Initialize digital sensor flags
#if SERIAL_DEBUG
    SerialTxStr(str_preboot_04);
    SerialTxStr(str_crlf);
    SerialTxStr(str_crlf);
#endif /* SERIAL_DEBUG */
    for (InputFlag digital_sensor = DHW_REQUEST_F; digital_sensor <= OVERHEAT_F; digital_sensor++) {
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
#if !(SERIAL_DEBUG)
    Dashboard(p_system, false);
#endif

    // Add a 2 seconds delay before activating the WDT
    _delay_ms(2000);
    // If the system freezes, reset the microcontroller after 8 seconds
    wdt_enable(WDTO_8S);

    // Set timer for gas valve modulation
    SetTimer(VALVE_OPEN_TIMER_ID, (unsigned long)VALVE_OPEN_TIMER_DURATION, RUN_ONCE_AND_HOLD);

    // Enable global interrupts
    sei();
    SetTickTimer();

    /*  ___________________
       |                   | 
       |     Main Loop     |
       |___________________|
    */
    for (;;) {
        // Reset the WDT
        wdt_reset();

        // Update digital input sensors status
        for (InputFlag digital_sensor = DHW_REQUEST_F; digital_sensor <= OVERHEAT_F; digital_sensor++) {
            CheckDigitalSensor(p_system, digital_sensor, p_debounce, false);
        }

        // Update analog input sensors status
        for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
            CheckAnalogSensor(p_system, p_buffer_pack, analog_sensor, false);
        }

        // If the pump is working, check delay counter to turn it off
        if (GetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F)) {
            if (!(p_system->pump_delay--)) {
                ClearFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F);
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
        if (GetFlag(p_system, INPUT_FLAGS, OVERHEAT_F)) {
            p_system->error = ERROR_001;
            p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
        }
#endif /* OVERHEAT_OVERRIDE */

        // Display updated status
#if !(SERIAL_DEBUG)
        Dashboard(p_system, false);
#endif

        // System FSM
        switch (p_system->system_state) {
            /*  ________________________
              |                         |
              |   System state -> OFF   |
              |_________________________|
            */
            case OFF: {
                // Verify that the flame sensor is off at this point, otherwise, there's a failure
                if (GetFlag(p_system, INPUT_FLAGS, FLAME_F)) {
                    GasOff(p_system);
                    p_system->error = ERROR_002;
                    p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
                    break;
                }
#if !(AIRFLOW_OVERRIDE)
                // If there isn't a fan test in progress, verify that the airflow sensor is off, otherwise, there's a failure
                if ((p_system->inner_step < OFF_3) && (GetFlag(p_system, INPUT_FLAGS, AIRFLOW_F))) {
                    ClearFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN_F);
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
                        if (!(delay--)) {                                   /* DLY_L_OFF_2 */
                            SetFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN_F); /* Turn exhaust fan on */
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
                        if (GetFlag(p_system, INPUT_FLAGS, AIRFLOW_F)) {
                            ClearFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN_F);
                            delay = DLY_L_OFF_4;
                            p_system->inner_step = OFF_4;
                        }
                        // Timeout: Airflow sensor didn't activate on time -> fan test failed
                        if (!(delay--)) { /* DLY_L_OFF_3 */
                            ClearFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN_F);
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
                            if (GetFlag(p_system, INPUT_FLAGS, AIRFLOW_F)) {
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
                    if (GetFlag(p_system, INPUT_FLAGS, FLAME_F)) {
                        p_system->error = ERROR_002;
                        p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
                    }
                }
                // If the water pump still has time to run before shutting
                // down, let it run until the delay counter reaches zero
                if (p_system->pump_delay > 0) {
                    SetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F);
                }
#if !(AIRFLOW_OVERRIDE)
                // Give the airflow sensor time before checking if it switches off when the fan gets turned off
                if (delay < (DLY_L_READY_1 - DLY_AIRFLOW_OFF)) {
                    // Verify that the airflow sensor is off at this point, otherwise, there's a failure
                    if (GetFlag(p_system, INPUT_FLAGS, AIRFLOW_F)) {
                        p_system->error = ERROR_003;
                        p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
                    }
                }
#endif /* AIRFLOW_OVERRIDE */
                // Check if there a DHW or CH request. If both are requested, DHW will have higher priority after ignition
                if ((GetFlag(p_system, INPUT_FLAGS, DHW_REQUEST_F)) || (GetFlag(p_system, INPUT_FLAGS, CH_REQUEST_F))) {
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
                if ((GetFlag(p_system, INPUT_FLAGS, DHW_REQUEST_F) == false) &&
                    (GetFlag(p_system, INPUT_FLAGS, CH_REQUEST_F) == false)) {
                    // Request canceled, turn actuators off and return to "ready" state
                    GasOff(p_system);
                    p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                    delay = DLY_L_READY_1;
                    p_system->inner_step = READY_1;
                    p_system->system_state = READY;
                    break;  // *** *** *** *** *** *** *** *** *** *** *** *** //
                }
                switch (p_system->inner_step) {
                    // .................................
                    // . Step IGNITING_1 : Turn fan on  .
                    // .................................
                    case IGNITING_1: {
                        if (!(delay--)) {                                   /* DLY_L_IGNITING_1 */
                            SetFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN_F); /* Turn exhaust fan on */
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
                        if (GetFlag(p_system, INPUT_FLAGS, AIRFLOW_F)) {
                            delay = DLY_L_IGNITING_3;
                            p_system->inner_step = IGNITING_3;
                        }
                        // Airflow sensor activation timeout -> ignition sequence canceled
                        if (!(delay--)) { /* DLY_L_IGNITING_2 */
                            ClearFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN_F);
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
                            SetFlag(p_system, OUTPUT_FLAGS, VALVE_S_F);
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
                                ClearFlag(p_system, OUTPUT_FLAGS, VALVE_2_F);
                                SetFlag(p_system, OUTPUT_FLAGS, VALVE_1_F);
                            } else {
                                ClearFlag(p_system, OUTPUT_FLAGS, VALVE_1_F);
                                SetFlag(p_system, OUTPUT_FLAGS, VALVE_2_F);
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
                            SetFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F);
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
                        if (GetFlag(p_system, INPUT_FLAGS, FLAME_F)) {
                            // Turn spark igniter off
                            ClearFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F);
                        }
#endif                                    /* FAST_FLAME_DETECTION */
                        if (!(delay--)) { /* DLY_L_IGNITING_6 */
                            // Increment ignition retry counter
                            p_system->ignition_retries++;
                            // Turn spark igniter off
                            ClearFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F);
                            // If the burner is lit on time
                            if (GetFlag(p_system, INPUT_FLAGS, FLAME_F)) {
                                // Turn spark igniter off
                                ClearFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F);
                                // Reset ignition retry counter
                                p_system->ignition_retries = 0;
                                // Hand over control to the requested service (DHW has higher priority)
                                if (GetFlag(p_system, INPUT_FLAGS, DHW_REQUEST_F)) {
                                    //delay = DLY_L_FLAME_MODULATION / 3;
                                    p_system->inner_step = DHW_ON_DUTY_1;
                                    p_system->system_state = DHW_ON_DUTY;
                                } else if (GetFlag(p_system, INPUT_FLAGS, CH_REQUEST_F)) {
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
                if (GetFlag(p_system, INPUT_FLAGS, FLAME_F) == false) {
                    // Turn all heat valves off except the valve 1
                    ModulateGas(p_system, VALVE_1);
                    delay = DLY_L_IGNITING_1;
                    p_system->inner_step = IGNITING_1;
                    p_system->system_state = IGNITING;
                    break;
                }
#if !(AIRFLOW_OVERRIDE)
                // Verify that the airflow sensor is on, otherwise, close gas and go to error
                if (GetFlag(p_system, INPUT_FLAGS, AIRFLOW_F) == false) {
                    GasOff(p_system); /* Close gas, turn igniter and fan off */
                    p_system->error = ERROR_007;
                    p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
                }
#endif /* AIRFLOW_OVERRIDE */
                // If the pump is on, halt it ...
                if (GetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F)) {
                    ClearFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F);
                }
                // Check if the DHW request is over
                if (GetFlag(p_system, INPUT_FLAGS, DHW_REQUEST_F) == false) {
                    // If there is a CH request active, modulate to valve 1 and
                    // hand over control to CH service state
                    if (GetFlag(p_system, INPUT_FLAGS, CH_REQUEST_F)) {
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
                } else {
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
                        for (uint8_t vt_check = 0; vt_check < GAS_MODULATOR_VALVES; vt_check++) {
                            heat_level_time_usage += heat_level[current_heat_level].valve_open_time[vt_check];
                        }
                        if (heat_level_time_usage != 100) {
#if LED_DEBUG
                            SetFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F);  // Heat level setting error, the sum of the opening time of all valves must be 100!
                            _delay_ms(5000);
                            ClearFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F);
#endif
#if SERIAL_DEBUG
                            SerialTxStr(str_heat_mod_03);
#endif
                            // FAIL-SAFE: One level auto cool down in case of heat cycle error
                            current_heat_level--;
                            cycle_in_progress = false;
                            // return 1; // At his point, it should jump to the error state
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
                            if (current_valve >= GAS_MODULATOR_VALVES) {
                                // Cycle end: Reset to first valve
#if LED_DEBUG
                                if (GetFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F)) { /* Toggle SPARK_IGNITER_F on each heat-cycle start */
                                    ClearFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F);
                                } else {
                                    SetFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F);
                                }
#endif
#if SERIAL_DEBUG
                                SerialTxStr(str_crlf);
#endif
                                cycle_in_progress = false;
#if HEAT_MODULATOR_DEMO
                                // DEMO MODE: loops through all heat levels, from lower to higher
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
                            ModulateGas(p_system, gas_modulator[current_valve].heat_valve);
#if SERIAL_DEBUG
                            SerialTxStr(str_heat_mod_06);
                            SerialTxStr(str_heat_mod_04);
                            SerialTxNum(valve_to_close + 1, DIGITS_1);
                            SerialTxStr(str_heat_mod_05);
                            SerialTxStr(str_heat_mod_04);
                            SerialTxNum(current_valve + 1, DIGITS_1);
                            SerialTxChr(32);
                            SerialTxNum((heat_level[current_heat_level].valve_open_time[current_valve] * HEAT_CYCLE_TIME / 100), DIGITS_FREE);
                            SerialTxStr(str_heat_mod_08);
#endif
                        }
                    }
                    //
                    // [ # # # ] DHW heat modulation code end [ # # # ]
                    //
                }
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
                        // If the flame sensor is off, and CH request is still active, check that gas valves 3 and 2 are closed and retry ignition
                        if (GetFlag(p_system, INPUT_FLAGS, FLAME_F) == false) {
                            // Turn all heat valves off except the valve 1
                            ModulateGas(p_system, VALVE_1);
                            delay = DLY_L_IGNITING_1;
                            p_system->inner_step = IGNITING_1;
                            p_system->system_state = IGNITING;
                        }
#if !(AIRFLOW_OVERRIDE)
                        // Verify that the airflow sensor is on, otherwise, close gas and go to error
                        if (GetFlag(p_system, INPUT_FLAGS, AIRFLOW_F) == false) {
                            GasOff(p_system); /* Close gas, turn igniter and fan off */
                            p_system->error = ERROR_007;
                            p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
                        }
#endif /* AIRFLOW_OVERRIDE */
                        // Turn CH water pump on
                        if (GetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F) == false) {
                            SetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F);
                        }
                        // Restart continuously the water pump shutdown timeout counter
                        p_system->pump_delay = DLY_WATER_PUMP_OFF;

                        // If there is a DHW request active, modulate and hand over control to DHW service
                        if (GetFlag(p_system, INPUT_FLAGS, DHW_REQUEST_F)) {
                            p_system->ch_on_duty_step = CH_ON_DUTY_1; /* Preserve current CH service step */
                            // ModulateGas(p_system, VALVE_1); /* Change to valve 1 before handing over control to DHW state */
                            delay = DLY_L_FLAME_MODULATION / 3;
                            p_system->inner_step = DHW_ON_DUTY_1;
                            p_system->system_state = DHW_ON_DUTY;
                        } else {
                            // Check if the CH request is over
                            if (GetFlag(p_system, INPUT_FLAGS, CH_REQUEST_F) == false) {
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
                            // Turn all heat valves off except the valve 2
                            ModulateGas(p_system, VALVE_2);
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
                        if (GetFlag(p_system, INPUT_FLAGS, FLAME_F)) {
                            GasOff(p_system);
                        }
                        // Turn CH water pump on
                        if (GetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F) == false) {
                            SetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F);
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
                        if (GetFlag(p_system, INPUT_FLAGS, DHW_REQUEST_F)) {
                            p_system->ch_on_duty_step = CH_ON_DUTY_2; /* Preserve current CH service step */
                            p_system->last_displayed_iflags = 0xFF;   /* Force a display dashboard refresh */
                            delay = DLY_L_IGNITING_1;
                            p_system->inner_step = IGNITING_1;
                            p_system->system_state = IGNITING;
                        } else {
                            // Check if the CH request is over
                            if (GetFlag(p_system, INPUT_FLAGS, CH_REQUEST_F) == false) {
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
                    for (InputFlag digital_sensor = DHW_REQUEST_F; digital_sensor <= OVERHEAT_F; digital_sensor++) {
                        CheckDigitalSensor(p_system, digital_sensor, p_debounce, false);
                    }
                    p_system->system_state = ERROR;
#if !(SERIAL_DEBUG)
                    Dashboard(p_system, true);
#endif
                    SetFlag(p_system, OUTPUT_FLAGS, LED_UI_F);
                    //ControlActuator(p_system, LED_UI_F, TURN_ON, false); /* true updates display on each pass */
                    SerialTxStr(str_crlf);
                    SerialTxStr(str_error_s);
                    SerialTxNum(p_system->error, DIGITS_3);
                    SerialTxStr(str_error_e);
                    SerialTxStr(str_crlf);
                    _delay_ms(500);
                    ClearFlag(p_system, OUTPUT_FLAGS, LED_UI_F);
                    //ControlActuator(p_system, LED_UI_F, TURN_OFF, false);
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
