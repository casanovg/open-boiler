/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: victoria-control.c (main code) for ATmega328
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-09 (Easter Quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#include "victoria-control.h"

// Main function
int main(void) {
    /* ___________________
      |                   | 
      |    Setup Block    |
      |___________________|
    */
    wdt_disable();  // Disable watch dog timer
    SerialInit();   // Initialize USART for serial communications (57600, N, 8, 1)

    // System gas modulator
    HeatModulator gas_modulator[] = {
        {VALVE_1, VALVE_1_F, 7000, 0.87, false},
        {VALVE_2, VALVE_2_F, 12000, 1.46, false},
        {VALVE_3, VALVE_3_F, 20000, 2.39, false}};

    //System state initialization
    SysInfo sys_info;
    SysInfo *p_system = &sys_info;
    p_system->system_mode = SYS_OFF;
    p_system->system_state = OFF;
    p_system->inner_step = OFF_1;
    p_system->input_flags = 0;
    p_system->output_flags = 0;
    p_system->last_displayed_iflags = 0;
    p_system->last_displayed_oflags = 0;
    p_system->error = ERROR_000;
    p_system->ignition_tries = 1;
    p_system->ch_on_duty_step = CH_ON_DUTY_1;
    p_system->cycle_in_progress = 0;
    p_system->current_heat_level = 0;
    p_system->current_valve = 0;
    p_system->pump_timer_memory = 0;
    p_system->ch_water_overheat = false;

    for (uint8_t valve = 0; valve < HEAT_MODULATOR_VALVES; valve++) {
        p_system->heat_modulator[valve] = gas_modulator[valve];
    }

    // Initialize ADC buffers
    AdcBuffers buffer_pack;
    AdcBuffers *p_buffer_pack = &buffer_pack;
    InitAdcBuffers(p_buffer_pack, BUFFER_LENGTH);

    // Initialize system flag byes
    InitFlags(p_system, INPUT_FLAGS);
    InitFlags(p_system, OUTPUT_FLAGS);

    // Initialize actuator controls
    for (OutputFlag device = EXHAUST_FAN_F; device <= LED_UI_F; device++) {
        InitActuator(p_system, device);
    }

    // Turn all actuators off
    for (OutputFlag device = EXHAUST_FAN_F; device <= LED_UI_F; device++) {
        ClearFlag(p_system, OUTPUT_FLAGS, device);
        _delay_ms(5);  // 5-millisecond blocking delay after turning each device off
    }

    // Initialize digital sensor flags
    for (InputFlag digital_sensor = DHW_REQUEST_F; digital_sensor <= OVERHEAT_F; digital_sensor++) {
        InitDigitalSensor(p_system, digital_sensor);
    }

    // Initialize analog sensor inputs
    for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
        InitAnalogSensor(p_system, analog_sensor);
    }

    // Pre-load analog sensor values
    for (uint8_t i = 0; i < BUFFER_LENGTH; i++) {
        for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
            CheckAnalogSensor(p_system, p_buffer_pack, analog_sensor, false);
        }
    }

#if SHOW_DASHBOARD
    // Show system dashboard
    Dashboard(p_system, false);
#else
    ClrScr();
    SerialTxStr(str_crlf);
    SerialTxStr(str_header_01);
    SerialTxStr(str_no_dashboard);
    SerialTxStr(str_crlf);
#endif  // SHOW_DASHBOARD

    // WDT resets the system if it becomes unresponsive
    _delay_ms(2000);      // Safety 2-second blocking delay before activating the WDT
    wdt_enable(WDTO_8S);  // If the system freezes, reset the microcontroller after 8 seconds

    // Set system-wide timers
    SetTimer(FSM_TIMER_ID, FSM_TIMER_DURATION, FSM_TIMER_MODE);     // Main finite state machine timer
    SetTimer(HEAT_TIMER_ID, HEAT_TIMER_DURATION, HEAT_TIMER_MODE);  // Heat modulator timer
    SetTimer(PUMP_TIMER_ID, 0, PUMP_TIMER_MODE);                    // Water pump timer

    // Enable global interrupts
    sei();
    SetTickTimer();
    /* ___________________
      |                   | 
      |     Main Loop     |
      |___________________|
    */
    for (;;) {
        // Reset the WDT
        wdt_reset();

        // Update digital input sensors status
        for (InputFlag digital_sensor = DHW_REQUEST_F; digital_sensor <= OVERHEAT_F; digital_sensor++) {
            CheckDigitalSensor(p_system, digital_sensor, false);
        }

        // Update analog input sensors status
        for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
            CheckAnalogSensor(p_system, p_buffer_pack, analog_sensor, false);
        }

        // If the CH water pump is on, check if its timer is finished to turn it off
        if (TimerFinished(PUMP_TIMER_ID)) {
            if (GetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F)) {
                ClearFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F);
            }
        }

        // DHW temperature sensor out of range -> Error 008
        if ((p_system->dhw_temperature <= ADC_MIN_THRESHOLD) || (p_system->dhw_temperature >= ADC_MAX_THRESHOLD)) {
            GasOff(p_system);
            p_system->error = ERROR_008;
            p_system->system_state = ERROR;  // >>>>> Next state -> ERROR
        }

        // CH temperature sensor out of range -> Error 009
        if ((p_system->ch_temperature <= ADC_MIN_THRESHOLD) || (p_system->ch_temperature >= ADC_MAX_THRESHOLD)) {
            GasOff(p_system);
            p_system->error = ERROR_009;
            p_system->system_state = ERROR;  // >>>>> Next state -> ERROR
        }

        // Unexpected CH water overtemperature detected -> Error 010
        if (p_system->ch_temperature < (CH_SETPOINT_HIGH - MAX_CH_TEMP_TOLERANCE)) {
            if (p_system->system_state == CH_ON_DUTY) {
                // If the system is running in CH mode, there is a system failure, stop all and indicate error
                GasOff(p_system);
                p_system->error = ERROR_010;
                p_system->system_state = ERROR;  // >>>>> Next state -> ERROR
            } else {
                // If the system is DHW mode, activate the pump until the CH hot water has flow off the exchanger.
                // NOTE: While in DWH mode the pump is off, so the water is not recirculating through the CH circuit.
                // The CH circuit water held inside the heat exchanger gets hot collaterally and can cause the
                //  bimetallic thermostat to detect overtemperature and disrupting the operation.
                p_system->ch_water_overheat = true;
                if (GetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F) == false) {
                    SetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F);
                }
            }
        } else {
            // If the system is DHW mode and the CH water overtemperature is no longer detected, turn the pump off
            if ((p_system->ch_temperature >= (CH_SETPOINT_HIGH - (MAX_CH_TEMP_TOLERANCE + 10))) && p_system->ch_water_overheat) {
                if (GetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F) && TimerFinished(PUMP_TIMER_ID)) {
                    ClearFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F);
                }
                p_system->ch_water_overheat = false;
            }
        }

#if !(OVERHEAT_OVERRIDE)
        // Verify that the overheat thermostat is not open, otherwise, there's a failure
        if (GetFlag(p_system, INPUT_FLAGS, OVERHEAT_F)) {
            p_system->error = ERROR_001;
            p_system->system_state = ERROR;  // >>>>> Next state -> ERROR
        }
#endif  // OVERHEAT_OVERRIDE

#if SHOW_DASHBOARD
        // Display updated status on system dashboard
        Dashboard(p_system, false);
#endif  // SHOW_DASHBOARD

        if (GetKnobPosition(p_system->system_mode, SYSTEM_MODE_STEPS) < SYS_OFF) {
            // System FSM
            switch (p_system->system_state) {
                /* _________________________
                  |                         |
                  |   System state -> OFF   |
                  |_________________________|
                */
                case OFF: {
                    // Verify that the flame sensor is off at this point, otherwise, there's a failure
                    if (GetFlag(p_system, INPUT_FLAGS, FLAME_F)) {
                        GasOff(p_system);
                        p_system->error = ERROR_002;
                        p_system->system_state = ERROR;  // >>>>> Next state -> ERROR
                        break;
                    }
#if !(AIRFLOW_OVERRIDE)
                    // If there isn't a fan test in progress, verify that the airflow sensor is off, otherwise, there's a failure
                    if ((p_system->inner_step < OFF_3) && (GetFlag(p_system, INPUT_FLAGS, AIRFLOW_F))) {
                        ClearFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN_F);
                        p_system->error = ERROR_003;
                        p_system->system_state = ERROR;  // >>>>> Next state -> ERROR
                        break;
                    }
#endif  // AIRFLOW_OVERRIDE
                    switch (p_system->inner_step) {
                        // .....................................
                        // . Step OFF_1 : Turn all devices off  .
                        // .....................................
                        case OFF_1: {
                            // Turn all actuators off, except the CH water pump
                            GasOff(p_system);
                            ResetTimerLapse(FSM_TIMER_ID, DLY_OFF_2);
                            //if (GetKnobPosition(p_system->system_mode, SYSTEM_MODE_STEPS) < SYS_OFF) {
                            p_system->inner_step = OFF_2;
                            //}
                            break;
                        }
                        // ..................................................
                        // . Step OFF_2 : Fan test in progress: turn fan on  .
                        // ..................................................
                        case OFF_2: {
#if (!(AIRFLOW_OVERRIDE) && !(FAN_TEST_OVERRIDE))
                            if (TimerFinished(FSM_TIMER_ID)) {                   // DLY_OFF_2
                                SetFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN_F);  // Turn exhaust fan on
                                ResetTimerLapse(FSM_TIMER_ID, DLY_OFF_3);
                                p_system->inner_step = OFF_3;
                            }
#else
                            p_system->inner_step = OFF_3;
#endif  // AIRFLOW_OVERRIDE && FAN_TEST_OVERRIDE
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
                                ResetTimerLapse(FSM_TIMER_ID, DLY_OFF_4);
                                p_system->inner_step = OFF_4;
                            }
                            // Timeout: Airflow sensor didn't activate on time -> fan test failed
                            if (TimerFinished(FSM_TIMER_ID)) {  // DLY_OFF_3
                                ClearFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN_F);
                                p_system->error = ERROR_004;
                                p_system->system_state = ERROR;
                            }
#else
                            // Airflow sensor check skipped
#if SHOW_DASHBOARD
                            p_system->last_displayed_iflags = 0xFF;  // Force a display dashboard refresh
#endif  // SHOW_DASHBOARD
                            ResetTimerLapse(FSM_TIMER_ID, DLY_OFF_4);
                            p_system->inner_step = OFF_4;
#endif  // AIRFLOW_OVERRIDE && FAN_TEST_OVERRIDE
                            break;
                        }
                        // .......................................................................
                        // . Step OFF_4 : Fan test in progress: check airflow sensor deactivation .
                        // .......................................................................
                        case OFF_4: {
                            if (TimerFinished(FSM_TIMER_ID)) {  // DLY_OFF_4 --- Let the fan to rev down after the test ---
#if (!(AIRFLOW_OVERRIDE) && !(FAN_TEST_OVERRIDE))
                                if (GetFlag(p_system, INPUT_FLAGS, AIRFLOW_F)) {
                                    p_system->error = ERROR_006;
                                    p_system->system_state = ERROR;
                                } else {
#if SHOW_DASHBOARD
                                    p_system->last_displayed_iflags = 0xFF;  // Force a display dashboard refresh
#endif                                                                       // SHOW_DASHBOARD
                                    ResetTimerLapse(FSM_TIMER_ID, (DLY_READY_1));
                                    p_system->inner_step = READY_1;
                                    p_system->system_state = READY;
                                }
#else
#if SHOW_DASHBOARD
                                p_system->last_displayed_iflags = 0xFF;  // Force a display dashboard refresh
#endif  // SHOW_DASHBOARD
                                ResetTimerLapse(FSM_TIMER_ID, DLY_READY_1);
                                p_system->inner_step = READY_1;
                                p_system->system_state = READY;
#endif  // AIRFLOW_OVERRIDE && FAN_TEST_OVERRIDE
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
                /* ___________________________
                  |                           |
                  |   System state -> READY   |
                  |___________________________|
                */
                case READY: {
                    // Give the flame sensor time before checking if it is off when the gas is closed
                    // Give the airflow sensor time before checking if it switches off when the fan gets turned off
                    //if (delay < (DLY_READY_1 - DLY_FLAME_OFF)) {
                    if (TimerFinished(FSM_TIMER_ID)) { /* DLY_READY_1 */
                        // Verify that the flame sensor is off at this point, otherwise, there's a failure
                        if (GetFlag(p_system, INPUT_FLAGS, FLAME_F)) {
                            p_system->error = ERROR_002;
                            p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
                        }
#if !(AIRFLOW_OVERRIDE)
                        // Verify that the airflow sensor is off at this point, otherwise, there's a failure
                        if (GetFlag(p_system, INPUT_FLAGS, AIRFLOW_F)) {
                            p_system->error = ERROR_003;
                            p_system->system_state = ERROR; /* >>>>> Next state -> ERROR */
                        }
#endif /* AIRFLOW_OVERRIDE */
                    }
                    // If the CH water pump is off, but it still has pending running time, turn it on
                    if (GetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F) == false) {
                        if (p_system->pump_timer_memory != 0) {
                            ResetTimerLapse(PUMP_TIMER_ID, p_system->pump_timer_memory);
                            if (GetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F) == false) {
                                SetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F);
                            }
                            p_system->pump_timer_memory = 0;
                        }
                    }

                    // Check if there a DHW or CH request. If both are requested, DHW will have higher priority after ignition
                    if ((GetFlag(p_system, INPUT_FLAGS, DHW_REQUEST_F)) || (GetFlag(p_system, INPUT_FLAGS, CH_REQUEST_F))) {
#if SHOW_DASHBOARD
                        p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
#endif                                                          // SHOW_DASHBOARD
                        ResetTimerLapse(FSM_TIMER_ID, DLY_IGNITING_1);
                        p_system->inner_step = IGNITING_1;
                        p_system->system_state = IGNITING;
                    }
                    if (TimerFinished(FSM_TIMER_ID)) { /* DLY_READY_1 */
                        ResetTimerLapse(FSM_TIMER_ID, DLY_READY_1);
#if SHOW_DASHBOARD
                        p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
#endif                                                          // SHOW_DASHBOARD
                    }
                    break;
                }
                /* ______________________________
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
#if SHOW_DASHBOARD
                        p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
#endif                                                          // SHOW_DASHBOARD
                        ResetTimerLapse(FSM_TIMER_ID, DLY_READY_1);
                        p_system->ignition_tries = 1;
                        p_system->inner_step = READY_1;
                        p_system->system_state = READY;
                        break;  // *** *** *** *** *** *** *** *** *** *** *** *** //
                    }
                    switch (p_system->inner_step) {
                        // .................................
                        // . Step IGNITING_1 : Turn fan on  .
                        // .................................
                        case IGNITING_1: {
                            if (TimerFinished(FSM_TIMER_ID)) {                  /* DLY_IGNITING_1 */
                                SetFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN_F); /* Turn exhaust fan on */
                                ResetTimerLapse(FSM_TIMER_ID, DLY_IGNITING_2);
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
                                ResetTimerLapse(FSM_TIMER_ID, DLY_IGNITING_3);
                                p_system->inner_step = IGNITING_3;
                            }
                            // Airflow sensor activation timeout -> ignition sequence canceled
                            if (TimerFinished(FSM_TIMER_ID)) { /* DLY_IGNITING_2 */
                                ClearFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN_F);
                                p_system->error = ERROR_004;
                                p_system->system_state = ERROR;
                            }
#else
                            // Airflow sensor check skipped
#if SHOW_DASHBOARD
                            p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
#endif  // SHOW_DASHBOARD
                            ResetTimerLapse(FSM_TIMER_ID, DLY_IGNITING_3);
                            p_system->inner_step = IGNITING_3;
#endif  // AIRFLOW_OVERRIDE
                            break;
                        }
                        // .............................................
                        // . Step IGNITING_3 : Open gas security valve  .
                        // .............................................
                        case IGNITING_3: {
                            if (TimerFinished(FSM_TIMER_ID)) { /* DLY_IGNITING_3 */
                                SetFlag(p_system, OUTPUT_FLAGS, VALVE_S_F);
                                ResetTimerLapse(FSM_TIMER_ID, DLY_IGNITING_4);
                                p_system->inner_step = IGNITING_4;
                            }
                            break;
                        }
                        // ......................................................
                        // . Step IGNITING_4 : Open gas valve 1 or 2 alternately .
                        // ......................................................
                        case IGNITING_4: {
                            if (TimerFinished(FSM_TIMER_ID)) { /* DLY_IGNITING_4 */
                                if ((GetFlag(p_system, OUTPUT_FLAGS, VALVE_1_F) == false) || (p_system->ignition_tries == 1)) {
                                    OpenHeatValve(p_system, VALVE_1);
                                } else {
                                    OpenHeatValve(p_system, VALVE_2);
                                }
                                ResetTimerLapse(FSM_TIMER_ID, DLY_IGNITING_5);
                                p_system->inner_step = IGNITING_5;
                            }
                            break;
                        }
                        // ..........................................
                        // . Step IGNITING_5 : Turn spark igniter on .
                        // ..........................................
                        case IGNITING_5: {
                            if (TimerFinished(FSM_TIMER_ID)) { /* DLY_IGNITING_5 */
                                SetFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F);
                                // Stretch flame detection timeout on each ignition retry
                                ResetTimerLapse(FSM_TIMER_ID, (DLY_IGNITING_6));
                                p_system->inner_step = IGNITING_6;
                            }
                            break;
                        }
                        // ...............................................................................
                        // . Step IGNITING_6 : Check flame and hand over control to the requested service .
                        // ...............................................................................
                        case IGNITING_6: {
                            // As soon as the flame is detected, the spark igniter is turned off
                            // and control is handed over to the requested service
                            if (GetFlag(p_system, INPUT_FLAGS, FLAME_F)) {
                                // Turn spark igniter off
                                if (GetFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F)) {
                                    ClearFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F);
                                }
                                // Reset ignition attempts counter
                                p_system->ignition_tries = 1;
                                // Hand over control to the requested service (DHW has higher priority)
                                if (GetFlag(p_system, INPUT_FLAGS, DHW_REQUEST_F)) {
                                    ResetTimerLapse(HEAT_TIMER_ID, HEAT_TIMER_DURATION);
#if SHOW_DASHBOARD
                                    ResetTimerLapse(FSM_TIMER_ID, DLY_DHW_ON_DUTY_LOOP);
#endif  // SHOW_DASHBOARD
                                    p_system->inner_step = DHW_ON_DUTY_1;
                                    p_system->system_state = DHW_ON_DUTY;
                                } else {
                                    if (GetFlag(p_system, INPUT_FLAGS, CH_REQUEST_F)) {
                                        ResetTimerLapse(HEAT_TIMER_ID, HEAT_TIMER_DURATION);
#if SHOW_DASHBOARD
                                        ResetTimerLapse(FSM_TIMER_ID, DLY_CH_ON_DUTY_LOOP);
#endif  //SHOW_DASHBOARD
                                        p_system->inner_step = CH_ON_DUTY_1;
                                        p_system->system_state = CH_ON_DUTY;
                                    } else {
                                        // Request canceled, turn actuators off and return to "ready" state
                                        GasOff(p_system);
                                        ResetTimerLapse(FSM_TIMER_ID, DLY_READY_1);
                                        p_system->inner_step = READY_1;
                                        p_system->system_state = READY;
                                    }
                                }
                            } else {
                                // Timer finished without detecting flame
                                if (TimerFinished(FSM_TIMER_ID)) {  // DLY_IGNITING_6
                                    // Increment ignition retry counter
                                    if (p_system->ignition_tries++ >= MAX_IGNITION_TRIES) {
                                        // If the number of ignition retries reached the maximum, close gas and indicate an error
                                        GasOff(p_system);
                                        // Reset ignition retry counter
                                        p_system->ignition_tries = 1;
                                        p_system->error = ERROR_005;
                                        p_system->system_state = ERROR;
                                    } else {
                                        // If there are retries to be tried, restart the ignition cycle with the new parameters
                                        if (GetFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F)) {
                                            ClearFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F);
                                        }
                                        ResetTimerLapse(FSM_TIMER_ID, DLY_IGNITING_4);
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
                                ResetTimerLapse(FSM_TIMER_ID, DLY_IGNITING_1);
                                p_system->inner_step = IGNITING_1;
                            }
                            break;
                        }
                    }
                    break;
                }
                /* _________________________________
                  |                                 |
                  |   System state -> DHW_ON_DUTY   |
                  |_________________________________|
                */
                case DHW_ON_DUTY: {
                    // If the flame sensor is off, check that gas valves 3 and 2 are closed and retry ignition
                    if (GetFlag(p_system, INPUT_FLAGS, FLAME_F) == false) {
                        // Turn all heat valves off except the valve 1
                        OpenHeatValve(p_system, VALVE_1);
                        ResetTimerLapse(FSM_TIMER_ID, DLY_IGNITING_1);
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
                    // If a CH water overtemperature is not detected, but the CH water pump is on, store the running time remaining and halt it ...
                    if (p_system->ch_water_overheat == false) {
                        if ((p_system->pump_timer_memory == 0) && GetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F)) {
                            p_system->pump_timer_memory = GetTimeLeft(PUMP_TIMER_ID);
                            ResetTimerLapse(PUMP_TIMER_ID, 0);
                            ClearFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F);
                        }
                    }
                    // Check if the DHW request is over
                    if (GetFlag(p_system, INPUT_FLAGS, DHW_REQUEST_F) == false) {
                        // If there is a CH request active, modulate to valve 1 and
                        // hand over control to CH service state
                        if (GetFlag(p_system, INPUT_FLAGS, CH_REQUEST_F)) {
#if SHOW_DASHBOARD
                            p_system->last_displayed_iflags = 0xFF;  // Force a display dashboard refresh
                            ResetTimerLapse(FSM_TIMER_ID, DLY_CH_ON_DUTY_LOOP);
#endif  // SHOW_DASHBOARD
                            p_system->inner_step = p_system->ch_on_duty_step;
                            p_system->system_state = CH_ON_DUTY;
                        } else {
                            // DHW request canceled, turn gas off and return to "ready" state
                            GasOff(p_system);
                            ResetTimerLapse(HEAT_TIMER_ID, HEAT_TIMER_DURATION);
                            ResetTimerLapse(FSM_TIMER_ID, DLY_READY_1);
                            p_system->inner_step = READY_1;
                            p_system->system_state = READY;
                        }
                    } else {
                        // ***************************************************************************************
                        //                                                                                         *
                        p_system->current_heat_level = GetKnobPosition(p_system->dhw_setting, DHW_SETTING_STEPS);  //*
                        ModulateHeat(p_system, p_system->current_heat_level, DHW_HEAT_CYCLE_TIME);                 //*
                        //                                                                                        *
                        // ***************************************************************************************
                    }
#if SHOW_DASHBOARD
#if AUTO_DHW_DSP_REFRESH
                    if (TimerFinished(FSM_TIMER_ID)) {  // DLY_DHW_ON_DUTY_1
                        ResetTimerLapse(FSM_TIMER_ID, DLY_DHW_ON_DUTY_LOOP);
                        p_system->last_displayed_iflags = 0xFF;  // Force a display dashboard refresh
                    }
#endif  // AUTO_DHW_DSP_REFRESH
#endif  // SHOW_DASHBOARD
                    break;
                }
                /* ________________________________
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
                                OpenHeatValve(p_system, VALVE_1);
                                ResetTimerLapse(FSM_TIMER_ID, DLY_IGNITING_1);
                                p_system->inner_step = IGNITING_1;
                                p_system->system_state = IGNITING;
                            }
#if !(AIRFLOW_OVERRIDE)
                            // Verify that the airflow sensor is on, otherwise, close gas and go to error
                            if (GetFlag(p_system, INPUT_FLAGS, AIRFLOW_F) == false) {
                                GasOff(p_system);  // Close gas, turn igniter and fan off
                                p_system->error = ERROR_007;
                                p_system->system_state = ERROR;  // >>>>> Next state -> ERROR
                            }
#endif /* AIRFLOW_OVERRIDE */
                            // Turn CH water pump on and reset its timer continuously to its full-time lapse
                            if (GetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F) == false) {
                                SetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F);
                            }
                            ResetTimerLapse(PUMP_TIMER_ID, PUMP_TIMER_DURATION);
                            p_system->pump_timer_memory = 0;
                            p_system->ch_water_overheat = false;

                            // If there is a DHW request active, modulate and hand over control to DHW service
                            if (GetFlag(p_system, INPUT_FLAGS, DHW_REQUEST_F)) {
                                p_system->ch_on_duty_step = CH_ON_DUTY_1;  // Preserve current CH service step
                                // OpenHeatValve(p_system, VALVE_1); // Change to valve 1 before handing over control to DHW state
                                ResetTimerLapse(HEAT_TIMER_ID, HEAT_TIMER_DURATION);
#if SHOW_DASHBOARD
                                ResetTimerLapse(FSM_TIMER_ID, DLY_DHW_ON_DUTY_LOOP);
#endif  // SHOW_DASHBOARD
                                p_system->inner_step = DHW_ON_DUTY_1;
                                p_system->system_state = DHW_ON_DUTY;
                            } else {
                                // Check if the CH request is over
                                if (GetFlag(p_system, INPUT_FLAGS, CH_REQUEST_F) == false) {
                                    // CH request canceled, turn gas off and return to "ready" state
                                    GasOff(p_system);
                                    ResetTimerLapse(HEAT_TIMER_ID, HEAT_TIMER_DURATION);
                                    ResetTimerLapse(FSM_TIMER_ID, DLY_READY_1);
                                    p_system->inner_step = READY_1;
                                    p_system->system_state = READY;
                                    break;
                                }
                            }
                            // While the CH water temperature is cooler than setpoint high, continue heating
                            // Otherwise, close gas and move on to CH_ON_DUTY_2 step
                            // NOTE: The temperature reading last bit is masked out to avoid oscillations
                            if ((p_system->ch_temperature & CH_TEMP_MASK) >= CH_SETPOINT_HIGH) {
                                // *************************************************************************************
                                //                                                                                       *
                                p_system->current_heat_level = GetKnobPosition(p_system->ch_setting, CH_SETTING_STEPS);  //*
                                ModulateHeat(p_system, p_system->current_heat_level, DHW_HEAT_CYCLE_TIME);               //*
                                //                                                                                       *
                                // *************************************************************************************

                            } else {
                                //Close gas
                                GasOff(p_system);
                                // NO NO NO Restart the water pump shutdown timeout counter
                                // NO NO NO p_system->pump_delay = DLY_WATER_PUMP_OFF;
                                p_system->inner_step = CH_ON_DUTY_2;
                            }
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
                            // If the CH water pump is off, but it still has pending running time, turn it on
                            if (GetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F) == false) {
                                if (p_system->pump_timer_memory != 0) {
                                    ResetTimerLapse(PUMP_TIMER_ID, p_system->pump_timer_memory);
                                    if (GetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F) == false) {
                                        SetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F);
                                    }
                                    p_system->pump_timer_memory = 0;
                                }
                            }
                            // If the CH water temperature is colder than setpoint low, ignite the burner,
                            // then go back to CH_ON_DUTY_1 step
                            // NOTE: The temperature reading last bit is masked out to avoid oscillations
                            if ((p_system->ch_temperature & CH_TEMP_MASK) >= CH_SETPOINT_LOW) {
                                p_system->inner_step = CH_ON_DUTY_1;
                                ResetTimerLapse(FSM_TIMER_ID, DLY_IGNITING_1);
                                p_system->inner_step = IGNITING_1;
                                p_system->system_state = IGNITING;
                                break;
                            }
                            // If there is a DHW request active, ignite and hand over control to DHW service
                            if (GetFlag(p_system, INPUT_FLAGS, DHW_REQUEST_F)) {
                                p_system->ch_on_duty_step = CH_ON_DUTY_2;  // Preserve current CH service step
#if SHOW_DASHBOARD
                                p_system->last_displayed_iflags = 0xFF;  // Force a display dashboard refresh
#endif                                                                   // SHOW_DASHBOARD
                                ResetTimerLapse(FSM_TIMER_ID, DLY_IGNITING_1);
                                p_system->inner_step = IGNITING_1;
                                p_system->system_state = IGNITING;
                            } else {
                                // Check if the CH request is over
                                if (GetFlag(p_system, INPUT_FLAGS, CH_REQUEST_F) == false) {
                                    // CH request canceled, turn gas off and return to "ready" state
                                    GasOff(p_system);
                                    ResetTimerLapse(FSM_TIMER_ID, DLY_READY_1);
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
#if SHOW_DASHBOARD
                                ResetTimerLapse(FSM_TIMER_ID, DLY_CH_ON_DUTY_LOOP);
#endif  // SHOW_DASHBOARD
                                p_system->inner_step = p_system->ch_on_duty_step;
                            }
                            break;
                        }
                    }
#if SHOW_DASHBOARD
#if AUTO_CH_DSP_REFRESH
                    if (TimerFinished(FSM_TIMER_ID)) {  // DLY_CH_ON_DUTY_1
                        ResetTimerLapse(FSM_TIMER_ID, DLY_CH_ON_DUTY_LOOP);
                        p_system->last_displayed_iflags = 0xFF;  // Force a display dashboard refresh
                    }
#endif  // AUTO_CH_DSP_REFRESH
#endif  // SHOW_DASHBOARD
                    break;
                }
                /* ___________________________
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
                            //CheckDigitalSensor(p_system, digital_sensor, p_debounce, false);
                            CheckDigitalSensor(p_system, digital_sensor, false);
                        }
                        p_system->system_state = ERROR;
#if SHOW_DASHBOARD
                        // Display updated status on system dashboard
                        Dashboard(p_system, true);
                        SerialTxStr(str_error_s);
                        SerialTxNum(p_system->error, DIGITS_3);
                        SerialTxStr(str_error_e);
                        SerialTxStr(str_crlf);
#endif  // SHOW_DASHBOARD
                        SetFlag(p_system, OUTPUT_FLAGS, LED_UI_F);
                        _delay_ms(500);  // 500-millisecond blocking delay before each error signaling
                        ClearFlag(p_system, OUTPUT_FLAGS, LED_UI_F);
                        _delay_ms(500);  // 500-millisecond blocking delay after each error signaling
                    }
                    // If there are not enough slots for the required system timers, the system must be halted!
                    if (p_system->error != 12) {
                        // End of error loop
                        // Next state -> OFF (reset error and try to resume service)
#if SHOW_DASHBOARD
                        p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
#endif                                                          // SHOW_DASHBOARD
                        p_system->error = ERROR_000;
                        p_system->inner_step = OFF_1;
                        p_system->system_state = OFF;
                    } else {
                        return 1;
                    }
                    break;
                }
                /* _____________________________
                  |                             |
                  |   System state -> Default   |
                  |_____________________________|
                */
                default: {
                    ResetTimerLapse(FSM_TIMER_ID, DLY_OFF_2);
                    p_system->inner_step = OFF_1;
                    p_system->system_state = OFF;
                    break;
                }

            }    /* System FSM end */
        } else { /* If the system is in OFF or RESET mode ... */
            //ClrScr();
            if (GetKnobPosition(p_system->system_mode, SYSTEM_MODE_STEPS) == SYS_OFF) {
                // System OFF mode indication
                GasOff(p_system);
                p_system->system_state = OFF;
                p_system->inner_step = OFF_1;
                for (int i = 0; i < 6; i++) {
                    ToggleFlag(p_system, OUTPUT_FLAGS, LED_UI_F);
                    _delay_ms(100);  // Blocking delay
                    SerialTxChr((char)46);
                }
                SerialTxChr((char)32);
            } else {
                // System RESET mode indication
                GasOff(p_system);
                p_system->system_state = OFF;
                p_system->inner_step = OFF_1;
                for (int i = 0; i < 14; i++) {
                    ToggleFlag(p_system, OUTPUT_FLAGS, LED_UI_F);
                    _delay_ms(50);  // Blocking delay
                    SerialTxChr((char)42);
                }
                SerialTxChr((char)32);
            }
            //ToggleFlag(p_system, OUTPUT_FLAGS, LED_UI_F);
            _delay_ms(125);  // Blocking delay

        } /* Big if end */

    } /* Main loop end */

    return 0;
}
