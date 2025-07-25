/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: victoria-control.c (main code) for ATmega328
 *  ........................................................
 *  Version: 0.6 "Juan" / 2019-09-22
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

    // Electromechanical switches debouncing
    DebounceSw debounce_sw;
    DebounceSw *p_debounce = &debounce_sw;
    debounce_sw.airflow_deb = DLY_DEBOUNCE_CH_REQ;
    debounce_sw.ch_request_deb = DLY_DEBOUNCE_AIRFLOW;

    // Delay variables
    uint16_t l_delay = 0;

    // Initialize USART for serial communications (38400, N, 8, 1)
    SerialInit();

    // Initialize actuator controls
    for (OutputFlag device = EXHAUST_FAN; device <= LED_UI; device++) {
        InitActuator(p_system, device);
    }

    // Turn all actuators off
    for (OutputFlag device = EXHAUST_FAN; device <= LED_UI; device++) {
        ClearFlag(p_system, OUTPUT_FLAGS, device);
        _delay_ms(5);
    }

    // Initialize digital sensor flags
    for (InputFlag digital_sensor = DHW_REQUEST; digital_sensor <= OVERHEAT; digital_sensor++) {
        InitDigitalSensor(p_system, digital_sensor);
    }

    // Initialize analog sensor inputs
    for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
        InitAnalogSensor(p_system, analog_sensor);
    }

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
            CheckAnalogSensor(p_system, analog_sensor, false);
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
                        l_delay = DLY_L_OFF_2;
                        p_system->inner_step = OFF_2;
                        break;
                    }
                    // ..................................................
                    // . Step OFF_2 : Fan test in progress: turn fan on  .
                    // ..................................................
                    case OFF_2: {
#if !(AIRFLOW_OVERRIDE)
                        if (!(l_delay--)) { /* DLY_L_OFF_2 */
                            SetFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN);  /* Turn exhaust fan on */
                            l_delay = DLY_L_OFF_3;
                            p_system->inner_step = OFF_3;
                        }
#else
                        p_system->inner_step = OFF_3;
#endif /* AIRFLOW_OVERRIDE */
                        break;
                    }
                    // .....................................................................
                    // . Step OFF_3 : Fan test in progress: check airflow sensor activation .
                    // .....................................................................
                    case OFF_3: {
#if !(AIRFLOW_OVERRIDE)
                        // Airflow sensor activated -> fan test successful
                        if ((p_system->input_flags >> AIRFLOW) & true) {
                            ClearFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN);
                            l_delay = DLY_L_OFF_4;
                            p_system->inner_step = OFF_4;
                        }
                        // Timeout: Airflow sensor didn't activate on time -> fan test failed
                        if (!(l_delay--)) { /* DLY_L_OFF_3 */
                            ClearFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN);
                            p_system->error = ERROR_004;
                            p_system->system_state = ERROR;
                        }
#else
                        // Airflow sensor check skipped
                        p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                        l_delay = DLY_L_OFF_4;
                        p_system->inner_step = OFF_4;
#endif /* AIRFLOW_OVERRIDE */
                        break;
                    }
                    // .......................................................................
                    // . Step OFF_4 : Fan test in progress: check airflow sensor deactivation .
                    // .......................................................................                   
                    case OFF_4: {
                        //LED_UI_PORT |= (1 << LED_UI_PIN);       // @@@@@
                        if (!(l_delay--)) { /* DLY_L_OFF_4 --- Let the fan to rev down --- */
                                            //LED_UI_PORT &= ~(1 << LED_UI_PIN);  // @@@@@
#if !(AIRFLOW_OVERRIDE)
                            if ((p_system->input_flags >> AIRFLOW) & true) {
                                p_system->error = ERROR_006;
                                p_system->system_state = ERROR;
                            } else {
                                p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                                l_delay = DLY_L_READY_1;
                                p_system->inner_step = READY_1;
                                p_system->system_state = READY;
                            }
#else
                            p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                            l_delay = DLY_L_READY_1;
                            p_system->inner_step = READY_1;
                            p_system->system_state = READY;
#endif /* AIRFLOW_OVERRIDE */
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
                if (l_delay < (DLY_L_READY_1 - DLY_FLAME_OFF)) {
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
                if (l_delay < (DLY_L_READY_1 - DLY_AIRFLOW_OFF)) {
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
                    l_delay = DLY_L_IGNITING_1;
                    p_system->inner_step = IGNITING_1;
                    p_system->system_state = IGNITING;
                }
                if (!(l_delay--)) { /* DLY_L_READY_1 */
                    l_delay = DLY_L_READY_1;
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
                if ((((p_system->input_flags >> DHW_REQUEST) & true) == false) && \
                     (((p_system->input_flags >> CH_REQUEST) & true) == false)) {
                    // Request canceled, turn actuators off and return to "ready" state
                    GasOff(p_system);
                    p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                    l_delay = DLY_L_READY_1;
                    p_system->inner_step = READY_1;
                    p_system->system_state = READY;
                }   
                switch (p_system->inner_step) {
                    // .................................
                    // . Step IGNITING_1 : Turn fan on  .
                    // .................................                    
                    case IGNITING_1: {
                        if (!(l_delay--)) { /* DLY_L_IGNITING_1 */
                            SetFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN); /* Turn exhaust fan on */
                            l_delay = DLY_L_IGNITING_2;
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
                            l_delay = DLY_L_IGNITING_3;
                            p_system->inner_step = IGNITING_3;
                        }
                        // Airflow sensor activation timeout -> ignition sequence canceled
                        if (!(l_delay--)) { /* DLY_L_IGNITING_2 */
                            ClearFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN);
                            p_system->error = ERROR_004;
                            p_system->system_state = ERROR;
                        }
#else
                        // Airflow sensor check skipped
                        p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                        l_delay = DLY_L_IGNITING_3;
                        p_system->inner_step = IGNITING_3;
#endif /* AIRFLOW_OVERRIDE */
                        break;
                    }
                    // .............................................
                    // . Step IGNITING_3 : Open gas security valve  .
                    // .............................................
                    case IGNITING_3: {
                        if (!(l_delay--)) { /* DLY_L_IGNITING_3 */
                            SetFlag(p_system, OUTPUT_FLAGS, VALVE_S);
                            l_delay = DLY_L_IGNITING_4;
                            p_system->inner_step = IGNITING_4;
                        }
                        break;
                    }
                    // ......................................................
                    // . Step IGNITING_4 : Open gas valve 1 or 2 alternately .
                    // ......................................................
                    case IGNITING_4: {
                        if (!(l_delay--)) { /* DLY_L_IGNITING_4 */
                            if (p_system->ignition_retries == 0) {
                                ClearFlag(p_system, OUTPUT_FLAGS, VALVE_2);
                                SetFlag(p_system, OUTPUT_FLAGS, VALVE_1);
                            } else {
                                ClearFlag(p_system, OUTPUT_FLAGS, VALVE_1);
                                SetFlag(p_system, OUTPUT_FLAGS, VALVE_2);
                            }
                            
                            l_delay = DLY_L_IGNITING_5;
                            p_system->inner_step = IGNITING_5;
                        }
                        break;
                    }
                    // ..........................................
                    // . Step IGNITING_5 : Turn spark igniter on .
                    // ..........................................
                    case IGNITING_5: {
                        if (!(l_delay--)) { /* DLY_L_IGNITING_5 */
                            SetFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER);
                            // Stretch flame detection timeout on each ignition retry
                            l_delay = DLY_L_IGNITING_6 * ((p_system->ignition_retries + 1) ^ 2);
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
#endif /* FAST_FLAME_DETECTION */
                        if (!(l_delay--)) { /* DLY_L_IGNITING_6 */
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
                                    l_delay = DLY_L_FLAME_MODULATION / 3;
                                    p_system->inner_step = DHW_ON_DUTY_1;
                                    p_system->system_state = DHW_ON_DUTY;
                                } else if ((p_system->input_flags >> CH_REQUEST) & true) {
                                    l_delay = DLY_L_CH_ON_DUTY_LOOP;
                                    p_system->inner_step = CH_ON_DUTY_1;
                                    p_system->system_state = CH_ON_DUTY;
                                } else {
                                    // Request canceled, turn actuators off and return to "ready" state
                                    GasOff(p_system);
                                    l_delay = DLY_L_READY_1;
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
                                    l_delay = DLY_L_IGNITING_4;
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
                            l_delay = DLY_L_IGNITING_1;
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
                    l_delay = DLY_L_IGNITING_1;
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
                        l_delay = DLY_L_CH_ON_DUTY_LOOP;
                        p_system->inner_step = p_system->ch_on_duty_step;
                        p_system->system_state = CH_ON_DUTY;
                    } else {
                        // DHW request canceled, turn gas off and return to "ready" state
                        GasOff(p_system);
                        l_delay = DLY_L_READY_1;
                        p_system->inner_step = READY_1;
                        p_system->system_state = READY;
                    }
                }
                //
                // [ # # # ] DHW heat modulation code  [ # # # ]
                //
                switch (p_system->inner_step) {
                    // .......................................
                    // . DHW_ON_DUTY_1 : Flame modulation 1/3 .
                    // .......................................                    
                    case DHW_ON_DUTY_1: {
                        // Close valve 1
                        ClearFlag(p_system, OUTPUT_FLAGS, VALVE_1);
                        // Close valve 3
                        ClearFlag(p_system, OUTPUT_FLAGS, VALVE_3);
                        // Open valve 2
                        SetFlag(p_system, OUTPUT_FLAGS, VALVE_2);
                        if (!(l_delay--)) { /* DLY_L_FLAME_MODULATION / 3 */
                            l_delay = DLY_L_FLAME_MODULATION / 3;
                            p_system->inner_step = DHW_ON_DUTY_2;
                        }
                        break;
                    }
                    // .......................................
                    // . DHW_ON_DUTY_2 : Flame modulation 2/3 .
                    // .......................................                    
                    case DHW_ON_DUTY_2: {
                        // Close valve 2
                        ClearFlag(p_system, OUTPUT_FLAGS, VALVE_2);
                        // Close valve 3
                        ClearFlag(p_system, OUTPUT_FLAGS, VALVE_3);
                        // Open valve 1
                        SetFlag(p_system, OUTPUT_FLAGS, VALVE_1);
                        if (!(l_delay--)) { /* DLY_L_FLAME_MODULATION / 3 */
                            l_delay = DLY_L_FLAME_MODULATION / 3;
                            p_system->inner_step = DHW_ON_DUTY_3;
                        }
                        break;
                    }
                    // .......................................
                    // . DHW_ON_DUTY_3 : Flame modulation 3/3 .
                    // .......................................                    
                    case DHW_ON_DUTY_3: {
                        // Close valve 2
                        ClearFlag(p_system, OUTPUT_FLAGS, VALVE_2);
                        // Close valve 3
                        ClearFlag(p_system, OUTPUT_FLAGS, VALVE_3);
                        // Open valve 1
                        SetFlag(p_system, OUTPUT_FLAGS, VALVE_1);
                        if (!(l_delay--)) { /* DLY_L_FLAME_MODULATION / 3 */
                            l_delay = DLY_L_FLAME_MODULATION / 3;
                            p_system->inner_step = DHW_ON_DUTY_1;
                        }
                        break;
                    }
                    // ........................
                    // . DHW_ON_DUTY : Default .
                    // ........................
                    default: {
                        if (p_system->system_state == DHW_ON_DUTY) {
                            l_delay = DLY_L_FLAME_MODULATION / 3;
                            p_system->inner_step = DHW_ON_DUTY_1;
                        }
                        break;
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
                            l_delay = DLY_L_IGNITING_1;
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
                            l_delay = DLY_L_FLAME_MODULATION / 3;
                            p_system->inner_step = DHW_ON_DUTY_1;
                            p_system->system_state = DHW_ON_DUTY;
                        } else {
                            // Check if the CH request is over
                            if (((p_system->input_flags >> CH_REQUEST) & true) == false) {
                                // CH request canceled, turn gas off and return to "ready" state
                                GasOff(p_system);
                                l_delay = DLY_L_READY_1;
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
                            l_delay = DLY_L_IGNITING_1;
                            p_system->inner_step = IGNITING_1;
                            p_system->system_state = IGNITING;
                            break;
                        }
                        // If there is a DHW request active, ignite and hand over control to DHW service
                        if ((p_system->input_flags >> DHW_REQUEST) & true) {
                            p_system->ch_on_duty_step = CH_ON_DUTY_2; /* Preserve current CH service step */
                            p_system->last_displayed_iflags = 0xFF; /* Force a display dashboard refresh */
                            l_delay = DLY_L_IGNITING_1;
                            p_system->inner_step = IGNITING_1;
                            p_system->system_state = IGNITING; 
                        } else {
                            // Check if the CH request is over
                            if (((p_system->input_flags >> CH_REQUEST) & true) == false) {
                                // CH request canceled, turn gas off and return to "ready" state
                                GasOff(p_system);
                                l_delay = DLY_L_READY_1;
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
                            l_delay = DLY_L_CH_ON_DUTY_LOOP;
                            p_system->inner_step = p_system->ch_on_duty_step;
                        }
                        break;
                    }
                }
                if (!(l_delay--)) { /* DLY_L_CH_ON_DUTY_1 */
                    l_delay = DLY_L_CH_ON_DUTY_LOOP;
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
                l_delay = DLY_L_OFF_2;
                p_system->inner_step = OFF_1;                
                p_system->system_state = OFF;
                break;
            }

        } /* System FSM end */

    } /* Main loop end */

    return 0;

}

// Function GasOff
void GasOff(SysInfo *p_sys) {
    // Turn spark igniter off
    ClearFlag(p_sys, OUTPUT_FLAGS, SPARK_IGNITER);
    _delay_ms(5);
    // Close gas valve 3
    ClearFlag(p_sys, OUTPUT_FLAGS, VALVE_3);
    _delay_ms(5);
    // Close gas valve 2
    ClearFlag(p_sys, OUTPUT_FLAGS, VALVE_2);
    _delay_ms(5);
    // Close gas valve 1
    ClearFlag(p_sys, OUTPUT_FLAGS, VALVE_1);
    _delay_ms(5);
    // Close gas security valve
    ClearFlag(p_sys, OUTPUT_FLAGS, VALVE_S);
    _delay_ms(5);
    // Turn exhaust fan off
    ClearFlag(p_sys, OUTPUT_FLAGS, EXHAUST_FAN);
}

// Function SystemRestart
void SystemRestart(void) {
    wdt_enable(WDTO_15MS);  /* Restart by activating the watchdog timer */
    for (;;) {};   
}

// Function InitAnalogSensor
void InitAnalogSensor(SysInfo *p_sys, AnalogInput analog_sensor) {
    ADMUX |= (1 << REFS0);                                /* reference voltage on AVCC */
    ACSR &= ~(1 << ACIE); /* Clear analog comparator IRQ flag */
    ACSR = (1 << ACD); /* Stop analog comparator */
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); /* ADC clock prescaler /128 */
    //ADMUX |= (1 << ADLAR); /* left-adjust result, return only 8 bits */
    ADCSRA |= (1 << ADEN); /* enable ADC */
    //ADCSRA |= (1 << ADATE); /* auto-trigger enable */
    //ADCSRA |= (1 << ADSC); /* start first conversion */
    switch (analog_sensor) {
        case DHW_TEMPERATURE: {
            p_sys->dhw_temperature = 0;
            break;
        }
        case CH_TEMPERATURE: {
            p_sys->ch_temperature = 0;
            break;
        }
        case DHW_SETTING: {
            p_sys->dhw_setting = 0;
            break;
        }
        case CH_SETTING: {
            p_sys->ch_setting = 0;
            break;
        }
        case SYSTEM_SETTING: {
            p_sys->system_setting = 0;
            break;
        }
        default:
            break;
    }
}

// Function CheckAnalogSensor
uint16_t CheckAnalogSensor(SysInfo *p_sys, AnalogInput analog_sensor, bool ShowDashboard) {
    ADMUX = (0xF0 & ADMUX) | analog_sensor;
    ADCSRA |= (1 << ADSC);
    loop_until_bit_is_clear(ADCSRA, ADSC);
    switch (analog_sensor) {
        case DHW_TEMPERATURE: {
            p_sys->dhw_temperature = (ADC & 0x3FF);
            break;
        }
        case CH_TEMPERATURE: {
            p_sys->ch_temperature = (ADC & 0x3FF);
            break;
        }
        case DHW_SETTING: {
            p_sys->dhw_setting = (ADC & 0x3FF);
            break;
        }
        case CH_SETTING: {
            p_sys->ch_setting = (ADC & 0x3FF);
            break;
        }
        case SYSTEM_SETTING: {
            p_sys->system_setting = (ADC & 0x3FF);
            break;
        }
        default: {
            break;
        }
    }
    if (ShowDashboard == true) {
        Dashboard(p_sys, false);
    }
    return (ADC & 0x3FF);
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

// Function InitDigitalSensor
void InitDigitalSensor(SysInfo *p_sys, InputFlag digital_sensor) {
    switch (digital_sensor) {
        case DHW_REQUEST: {
            DHW_RQ_DDR &= ~(1 << DHW_RQ_PIN); /* Set DHW request pin as input */
            DHW_RQ_PORT |= (1 << DHW_RQ_PIN); /* Activate pull-up resistor on this pin */
            break;
        }
        case CH_REQUEST: {
            CH_RQ_DDR &= ~(1 << CH_RQ_PIN); /* Set CH request pin as input */
            CH_RQ_PORT |= (1 << CH_RQ_PIN); /* Activate pull-up resistor on this pin */
            break;
        }
        case AIRFLOW: {
            AIRFLOW_DDR &= ~(1 << AIRFLOW_PIN); /* Set Airflow detector pin as input */
            AIRFLOW_PORT |= (1 << AIRFLOW_PIN); /* Activate pull-up resistor on this pin */
            break;
        }
        case FLAME: {
            FLAME_DDR &= ~(1 << FLAME_PIN); /* Set Flame detector pin as input */
            //FLAME_PORT &= ~(1 << FLAME_PIN); /* Deactivate pull-up resistor on this pin */
            break;
        }
        case OVERHEAT: {
            OVERHEAT_DDR &= ~(1 << OVERHEAT_PIN); /* Set Overheat detector pin as input */
            //OVERHEAT_PORT &= ~(1 << OVERHEAT_PIN); /* Deactivate pull-up resistor on this pin */
            break;
        }
    }
    ClearFlag(p_sys, INPUT_FLAGS, digital_sensor);
}

// Function CheckDigitalSensor
bool CheckDigitalSensor(SysInfo *p_sys, InputFlag digital_sensor, DebounceSw *p_deb, bool ShowDashboard) {
    switch (digital_sensor) {
        case DHW_REQUEST: { /* DHW request: Active = low, Inactive = high */
            if ((DHW_RQ_PINP >> DHW_RQ_PIN) & true) {
                ClearFlag(p_sys, INPUT_FLAGS, DHW_REQUEST);
            } else {
                SetFlag(p_sys, INPUT_FLAGS, DHW_REQUEST);
            }
            return ((p_sys->input_flags >> DHW_REQUEST) & true);
        }
        case CH_REQUEST: { /* CH request: Active = low, Inactive = high (bimetallic room thermostat) */
            // CH request switch debouncing
            if (((p_sys->input_flags >> CH_REQUEST) & true) == ((CH_RQ_PINP >> CH_RQ_PIN) & true)) {
                if (!(p_deb->ch_request_deb--)) {
                    p_deb->ch_request_deb = DLY_DEBOUNCE_CH_REQ;
                    if ( ((p_sys->input_flags >> CH_REQUEST) & true) == ((CH_RQ_PINP >> CH_RQ_PIN) & true) ) {
                        p_sys->input_flags ^= (1 << CH_REQUEST);
                    }
                }
            }
            return ((p_sys->input_flags >> CH_REQUEST) & true);
        }
        case AIRFLOW: { /* Flue air flow sensor: Active = low, Inactive = high (flue air pressure switch) */
            // Airflow switch debouncing
            if (((p_sys->input_flags >> AIRFLOW) & true) == ((AIRFLOW_PINP >> AIRFLOW_PIN) & true)) {
                if (!(p_deb->airflow_deb--)) {
                    p_deb->airflow_deb = DLY_DEBOUNCE_AIRFLOW;
                    if ( ((p_sys->input_flags >> AIRFLOW) & true) == ((AIRFLOW_PINP >> AIRFLOW_PIN) & true) ) {
                        p_sys->input_flags ^= (1 << AIRFLOW);
                    }
                }
            }        
            return ((p_sys->input_flags >> AIRFLOW) & true);
        }
        case FLAME: { /* Flame sensor: Active = high, Inactive = low. IT NEEDS EXTERNAL PULL-DOWN RESISTOR !!! */
            if ((FLAME_PINP >> FLAME_PIN) & true) {
                SetFlag(p_sys, INPUT_FLAGS, FLAME);
#if LED_UI_FOR_FLAME
                SetFlag(p_sys, OUTPUT_FLAGS, LED_UI);
#endif /* LED_UI_FOR_FLAME */
            } else {
                ClearFlag(p_sys, INPUT_FLAGS, FLAME);
#if LED_UI_FOR_FLAME
                ClearFlag(p_sys, OUTPUT_FLAGS, LED_UI);
#endif /* LED_UI_FOR_FLAME */
            }
            return ((p_sys->input_flags >> FLAME) & true);
        }
        case OVERHEAT: { /* Overheat thermostat: Active = high, Inactive = low. ACTIVE INDICATES OVERTEMPERATURE !!! */
            if ((OVERHEAT_PINP >> OVERHEAT_PIN) & true) {
                ClearFlag(p_sys, INPUT_FLAGS, OVERHEAT);
            } else {
                SetFlag(p_sys, INPUT_FLAGS, OVERHEAT);
                p_sys->input_flags |= (1 << OVERHEAT);
            }
            return ((p_sys->input_flags >> OVERHEAT) & true);
        }
    }
    if (ShowDashboard == true) {
        Dashboard(p_sys, false);
    }
    return 0;
}

// Function InitActuator
void InitActuator(SysInfo *p_sys, OutputFlag device) {
    switch (device) {
        case EXHAUST_FAN: {
            FAN_DDR |= (1 << FAN_PIN);   /* Set exhaust fan pin as output */
            FAN_PORT &= ~(1 << FAN_PIN); /* Set exhaust fan pin low (inactive) */
            break;
        }
        case WATER_PUMP: {
            PUMP_DDR |= (1 << PUMP_PIN);   /* Set water pump pin as output */
            PUMP_PORT &= ~(1 << PUMP_PIN); /* Set water pump pin low (inactive) */
            break;
        }
        case SPARK_IGNITER: {
            SPARK_DDR |= (1 << SPARK_PIN);  /* Set spark igniter pin as output */
            SPARK_PORT |= (1 << SPARK_PIN); /* Set spark igniter pin high (inactive) */
            break;
        }
        case VALVE_S: {
            VALVE_S_DDR |= (1 << VALVE_S_PIN);   /* Set security valve pin as output */
            VALVE_S_PORT &= ~(1 << VALVE_S_PIN); /* Set security valve pin low (inactive) */
            break;
        }
        case VALVE_1: {
            VALVE_1_DDR |= (1 << VALVE_1_PIN);   /* Set security valve pin as output */
            VALVE_1_PORT &= ~(1 << VALVE_1_PIN); /* Set security valve pin low (inactive) */
            break;
        }
        case VALVE_2: {
            VALVE_2_DDR |= (1 << VALVE_2_PIN);   /* Set security valve pin as output */
            VALVE_2_PORT &= ~(1 << VALVE_2_PIN); /* Set security valve pin low (inactive) */
            break;
        }
        case VALVE_3: {
            VALVE_3_DDR |= (1 << VALVE_3_PIN);   /* Set security valve pin as output */
            VALVE_3_PORT &= ~(1 << VALVE_3_PIN); /* Set security valve pin low (inactive) */
            break;
        }
        case LED_UI: {
            LED_UI_DDR |= (1 << LED_UI_PIN);   /* Set LED UI pin as output */
            LED_UI_PORT &= ~(1 << LED_UI_PIN); /* Set LED UI pin low (inactive) */
            break;
        }
        default: {
            break;
        }
    }
    ClearFlag(p_sys, OUTPUT_FLAGS, device); /* Clear actuator flags */
}

// Function ControlActuator
void ControlActuator(SysInfo *p_sys, OutputFlag device, HwSwitch command, bool ShowDashboard) {
    switch (device) {
        case EXHAUST_FAN: {
            if (command == TURN_ON) {
                FAN_PORT |= (1 << FAN_PIN); /* Set exhaust fan pin high (active) */
            } else {
                FAN_PORT &= ~(1 << FAN_PIN); /* Set exhaust fan pin low (inactive) */
            }
            break;
        }
        case WATER_PUMP: {
            if (command == TURN_ON) {
                PUMP_PORT |= (1 << PUMP_PIN); /* Set water pump pin high (active) */
            } else {
                PUMP_PORT &= ~(1 << PUMP_PIN); /* Set water pump pin low (inactive) */
            }
            break;
        }
        case SPARK_IGNITER: {
            if (command == TURN_ON) {
                SPARK_PORT &= ~(1 << SPARK_PIN); /* Set spark igniter pin low (active) */
            } else {
                SPARK_PORT |= (1 << SPARK_PIN); /* Set spark igniter pin high (inactive) */
            }
            break;
        }
        case VALVE_S: {
            if (command == TURN_ON) {
                VALVE_S_PORT |= (1 << VALVE_S_PIN); /* Set security valve pin high (active) */
            } else {
                VALVE_S_PORT &= ~(1 << VALVE_S_PIN); /* Set security valve pin low (inactive) */
            }
            break;
        }
        case VALVE_1: {
            if (command == TURN_ON) {
                VALVE_1_PORT |= (1 << VALVE_1_PIN); /* Set valve 1 pin high (active) */
            } else {
                VALVE_1_PORT &= ~(1 << VALVE_1_PIN); /* Set valve 1 pin low (inactive) */
            }
            break;
        }
        case VALVE_2: {
            if (command == TURN_ON) {
                VALVE_2_PORT |= (1 << VALVE_2_PIN); /* Set valve 2 pin high (active) */
            } else {
                VALVE_2_PORT &= ~(1 << VALVE_2_PIN); /* Set valve 2 pin low (inactive) */
            }
            break;
        }
        case VALVE_3: {
            if (command == TURN_ON) {
                VALVE_3_PORT |= (1 << VALVE_3_PIN); /* Set valve 3 pin high (active) */
            } else {
                VALVE_3_PORT &= ~(1 << VALVE_3_PIN); /* Set valve 3 pin low (inactive) */
            }
            break;
        }
        case LED_UI: {
            if (command == TURN_ON) {
                LED_UI_PORT |= (1 << LED_UI_PIN); /* Set LED UI pin high (active) */
            } else {
                LED_UI_PORT &= ~(1 << LED_UI_PIN); /* Set LED UI pin low (inactive) */
            }
            break;
        }
        default: {
            break;
        }
    }
    // If called directly, synchronize output flags with hardware status
    if (command == TURN_ON) {
        if (!(GetFlag(p_sys, OUTPUT_FLAGS, device))) {
            SetFlag(p_sys, OUTPUT_FLAGS, device); /* Set actuator flags */
        }
    } else {
        if (GetFlag(p_sys, OUTPUT_FLAGS, device)) {
            ClearFlag(p_sys, OUTPUT_FLAGS, device); /* Clear actuator flags */
        }
    }
    if (ShowDashboard == true) {
        Dashboard(p_sys, false);
    }
}

// Function SerialInit
void SerialInit(void) {
    UBRR0H = (uint8_t)(BAUD_PRESCALER >> 8);
    UBRR0L = (uint8_t)(BAUD_PRESCALER);
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (3 << UCSZ00);
}

// Function SerialRxChr
unsigned char SerialRxChr(void) {
    while (!(UCSR0A & (1 << RXC0))) {
    };
    return UDR0;
}

// Function SerialTxChr
void SerialTxChr(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0))) {
    };
    UDR0 = data;
}

// Function SerialTxNum
void SerialTxNum(uint32_t data, DigitLength digits) {
#define DATA_LNG 7
    char str[DATA_LNG] = {0};
    switch (digits) {
        case DIGITS_1: {
            sprintf(str, "%01u", (unsigned int)data);
            break;
        }
        case DIGITS_2: {
            sprintf(str, "%02u", (unsigned int)data);
            break;
        }
        case DIGITS_3: {
            sprintf(str, "%03u", (unsigned int)data);
            break;
        }
        case DIGITS_4: {
            sprintf(str, "%04u", (unsigned int)data);
            break;
        }
        case DIGITS_5: {
            sprintf(str, "%05u", (unsigned int)data);
            break;
        }
        case DIGITS_6: {
            sprintf(str, "%06u", (unsigned int)data);
            break;
        }
        case DIGITS_7: {
            sprintf(str, "%07lu", data);
            break;
        }                
        case DIGITS_FREE:
        default : {
            sprintf(str, "%u", (unsigned int)data);
            break;
        }
    }
    for (int i = 0; i < DATA_LNG; i++) {
        while (!(UCSR0A & (1 << UDRE0))) {
        };
        UDR0 = str[i];
    }
}

// Function SerialTxStr
void SerialTxStr(const __flash char *ptr_string) {
    for (uint8_t k = 0; k < strlen_P(ptr_string); k++) {
        char my_char = pgm_read_byte_near(ptr_string + k);
        SerialTxChr(my_char);
    }
}

// Function Dashboard
void Dashboard(SysInfo *p_sys, bool force_display) {
#define DASH_WIDTH 65
#define H_ELINE 46
#define H_ILINE 46
#define V_LINE 46

    if (force_display |
        (p_sys->input_flags != p_sys->last_displayed_iflags) |
        (p_sys->output_flags != p_sys->last_displayed_oflags)) {
        p_sys->last_displayed_iflags = p_sys->input_flags;
        p_sys->last_displayed_oflags = p_sys->output_flags;

        ClrScr();

        DrawLine(DASH_WIDTH, H_ELINE); /* Dashed line (= 61) */
        SerialTxStr(str_crlf);         /* CR + new line */

        SerialTxChr(V_LINE); /* Horizontal separator (|) */
        SerialTxStr(str_header_01);
        SerialTxStr(str_header_02);

        // Mode display
        switch (p_sys->system_state) {
            case OFF: {
                SerialTxStr(str_mode_00);
                break;
            }
            case READY: {
                SerialTxStr(str_mode_10);
                break;
            }
            case IGNITING: {
                SerialTxStr(str_mode_20);
                break;
            }
            case DHW_ON_DUTY: {
                SerialTxStr(str_mode_30);
                break;
            }
            case CH_ON_DUTY: {
                SerialTxStr(str_mode_40);
                break;
            }
            case ERROR: {
                SerialTxStr(str_mode_100);
                break;
            }
        }

        SerialTxChr(V_LINE); /* Horizontal separator (|) */
        SerialTxChr(32);     /* Space (_) */

        DrawLine(DASH_WIDTH - 4, H_ILINE); /* Dotted line */
        SerialTxChr(32);
        SerialTxChr(V_LINE); /* Horizontal separator (|) */

        SerialTxStr(str_crlf); /* CR + new line */

        SerialTxChr(V_LINE); /* Horizontal separator (|) */
        SerialTxChr(32);     /* Space (_) */

        // Input flags
        SerialTxStr(str_iflags);
        SerialTxNum(p_sys->input_flags, DIGITS_3);

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // DHW temperature
        SerialTxStr(str_lit_13);
        SerialTxNum(p_sys->dhw_temperature, DIGITS_4);
        //@@@@@ SerialTxNum((p_sys->ch_setting * 1.1), DIGITS_4);

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // CH temperature
        SerialTxStr(str_lit_14);
        SerialTxNum(p_sys->ch_temperature, DIGITS_4);

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Overheat
#if !(OVERHEAT_OVERRIDE)
        SerialTxStr(str_lit_04);
#else
        SerialTxStr(str_lit_04_override);
#endif /* OVERHEAT_OVERRIDE */
        if (GetFlag(p_sys, INPUT_FLAGS, OVERHEAT)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(V_LINE); /* Horizontal separator (|) */

        SerialTxStr(str_crlf); /* CR + new line */

        SerialTxChr(V_LINE); /* Horizontal separator (|) */
        SerialTxChr(32);     /* Space (_) */

        // DHW Request
        SerialTxStr(str_lit_00);
        if (GetFlag(p_sys, INPUT_FLAGS, DHW_REQUEST)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        //CH Request
        SerialTxStr(str_lit_01);
        if (GetFlag(p_sys, INPUT_FLAGS, CH_REQUEST)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Airflow
#if !(AIRFLOW_OVERRIDE)
        SerialTxStr(str_lit_02);
#else
        SerialTxStr(str_lit_02_override);
#endif /* AIRFLOW_OVERRIDE */
        if (GetFlag(p_sys, INPUT_FLAGS, AIRFLOW)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Flame
        SerialTxStr(str_lit_03);
        if (GetFlag(p_sys, INPUT_FLAGS, FLAME)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(V_LINE); /* Horizontal separator (|) */

        SerialTxStr(str_crlf); /* CR + new line */

        SerialTxChr(V_LINE); /* Horizontal separator (|) */
        SerialTxChr(32);     /* Space (_) */

        DrawLine(DASH_WIDTH - 4, H_ILINE); /* Dotted line */
        SerialTxChr(32);                   /* Space (_) */
        SerialTxChr(V_LINE);               /* Horizontal separator (|) */

        SerialTxStr(str_crlf); /* CR + new line */

        SerialTxChr(V_LINE); /* Horizontal separator (|) */
        SerialTxChr(32);     /* Space (_) */

        SerialTxStr(str_lit_18);
        SerialTxChr(32); /* Space (_) */
        SerialTxStr(str_lit_15);
        SerialTxNum(p_sys->dhw_setting, DIGITS_4);
        //@@@@@ SerialTxNum((p_sys->ch_setting / 1.1), DIGITS_4);
        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */
        SerialTxStr(str_lit_16);
        SerialTxNum(p_sys->ch_setting, DIGITS_4);
        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */
        SerialTxStr(str_lit_17);
        SerialTxNum(p_sys->system_setting, DIGITS_4);

        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(V_LINE); /* Horizontal separator (|) */

        SerialTxStr(str_crlf); /* CR + new line */

        SerialTxChr(V_LINE); /* Horizontal separator (|) */
        SerialTxChr(32);     /* Space (_) */

        DrawLine(DASH_WIDTH - 4, H_ILINE); /* Dotted line */
        SerialTxChr(32);
        SerialTxChr(V_LINE); /* Horizontal separator (|) */

        SerialTxStr(str_crlf);

        SerialTxChr(V_LINE); /* Horizontal separator (|) */
        SerialTxChr(32);     /* Space (_) */

        SerialTxStr(str_oflags);
        SerialTxNum(p_sys->output_flags, DIGITS_3);

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Exhaust fan
        SerialTxStr(str_lit_05);
        if (GetFlag(p_sys, OUTPUT_FLAGS, EXHAUST_FAN)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Water pump
        SerialTxStr(str_lit_06);
        if (GetFlag(p_sys, OUTPUT_FLAGS, WATER_PUMP)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Spark igniter
        SerialTxStr(str_lit_07);
        if (GetFlag(p_sys, OUTPUT_FLAGS, SPARK_IGNITER)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // LED UI
        SerialTxStr(str_lit_12);
        if (GetFlag(p_sys, OUTPUT_FLAGS, LED_UI)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(V_LINE); /* Horizontal separator (|) */

        SerialTxStr(str_crlf); /* CR + new line */

        SerialTxChr(V_LINE); /* Horizontal separator (|) */
        SerialTxChr(32);     /* Space (_) */

        // Security valve
        SerialTxStr(str_lit_08);
        if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_S)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Valve 1
        SerialTxStr(str_lit_09);
        if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_1)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Valve 2
        SerialTxStr(str_lit_10);
        if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_2)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Valve 3
        SerialTxStr(str_lit_11);
        if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_3)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32);     /* Space (_) */
        SerialTxChr(V_LINE); /* Horizontal separator (|) */

        SerialTxStr(str_crlf);         /* CR + new line */
        DrawLine(DASH_WIDTH, H_ELINE); /* Dashed line */
        SerialTxStr(str_crlf);         /* CR + new line */

#if SHOW_PUMP_TIMER
        SerialTxStr(str_crlf);
        SerialTxStr(str_wptimer);
        SerialTxNum(p_sys->pump_delay, DIGITS_7);
        SerialTxStr(str_crlf);
#endif /* SHOW_PUMP_TIMER */

    }
}

// Function DrawDashedLine
void DrawLine(uint8_t length, char line_char) {
    for (uint8_t i = 0; i < length; i++) {
        //SerialTxChr(61);
        SerialTxChr(line_char);
    }
}

// Function clear screen
void ClrScr(void) {
    for (uint8_t i = 0; i < (sizeof(clr_ascii) / sizeof(clr_ascii[0])); i++) {
        SerialTxChr(clr_ascii[i]);
    }
}
