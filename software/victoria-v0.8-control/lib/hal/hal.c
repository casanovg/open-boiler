/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: hal.c (boiler hardware abstraction layer library)
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-10 (Easter quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#include "hal.h"

#include <serial-ui.h>

// Function SystemRestart: Restarts the system by activating the watchdog timer
void SystemRestart(void) {
    wdt_enable(WDTO_15MS);
    for (;;) {
    };
}

// Function InitFlags: Assigns 0 (false) to all bits of a system flag byte
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

// Function SetFlag: Assigns 1 (true) to a given system flag and turns its associate actuator's pin on
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

// Function ClearFlag: Assigns 0 (false) to a given system flag and turns its associate actuator's pin off
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

// Function GetFlag: Returns the binary value of a given system flag 
bool GetFlag(SysInfo *p_sys, FlagsType flags_type, uint8_t flag_position) {
    bool flag = 0;
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

// Function InitDigitalSensor: Initializes a digital sensor's input pin
void InitDigitalSensor(SysInfo *p_sys, InputFlag digital_sensor) {
    switch (digital_sensor) {
        case DHW_REQUEST_F: {
            DHW_RQ_DDR &= ~(1 << DHW_RQ_PIN); /* Set DHW request pin as input */
            DHW_RQ_PORT |= (1 << DHW_RQ_PIN); /* Activate pull-up resistor on this pin */
            break;
        }
        case CH_REQUEST_F: {
            CH_RQ_DDR &= ~(1 << CH_RQ_PIN); /* Set CH request pin as input */
            CH_RQ_PORT |= (1 << CH_RQ_PIN); /* Activate pull-up resistor on this pin */
            break;
        }
        case AIRFLOW_F: {
            AIRFLOW_DDR &= ~(1 << AIRFLOW_PIN); /* Set Airflow detector pin as input */
            AIRFLOW_PORT |= (1 << AIRFLOW_PIN); /* Activate pull-up resistor on this pin */
            break;
        }
        case FLAME_F: {
            FLAME_DDR &= ~(1 << FLAME_PIN); /* Set Flame detector pin as input */
            //FLAME_PORT &= ~(1 << FLAME_PIN); /* Deactivate pull-up resistor on this pin */
            break;
        }
        case OVERHEAT_F: {
            OVERHEAT_DDR &= ~(1 << OVERHEAT_PIN); /* Set Overheat detector pin as input */
            //OVERHEAT_PORT &= ~(1 << OVERHEAT_PIN); /* Deactivate pull-up resistor on this pin */
            break;
        }
    }
    ClearFlag(p_sys, INPUT_FLAGS, digital_sensor);
}

// Function CheckDigitalSensor: Returns the binary value of a given digital sensor and updates its associated flag
bool CheckDigitalSensor(SysInfo *p_sys, InputFlag digital_sensor, DebounceSw *p_deb, bool ShowDashboard) {
    switch (digital_sensor) {
        case DHW_REQUEST_F: { /* DHW request: Active = low, Inactive = high */
            if ((DHW_RQ_PINP >> DHW_RQ_PIN) & true) {
                ClearFlag(p_sys, INPUT_FLAGS, DHW_REQUEST_F);
            } else {
                SetFlag(p_sys, INPUT_FLAGS, DHW_REQUEST_F);
            }
            return ((p_sys->input_flags >> DHW_REQUEST_F) & true);
        }
        // case CH_REQUEST_F: { /* DHW request: Active = low, Inactive = high */
        //     // CH request switch @@@@@@ @@@@@@ @@@@@@ NO DEBOUNCE!!! @@@@@@ @@@@@@ @@@@@@
        //     if ((CH_RQ_PINP >> CH_RQ_PIN) & true) {
        //         ClearFlag(p_sys, INPUT_FLAGS, CH_REQUEST_F);
        //     } else {
        //         SetFlag(p_sys, INPUT_FLAGS, CH_REQUEST_F);
        //     }
        //     return ((p_sys->input_flags >> CH_REQUEST_F) & true);
        // }
        case CH_REQUEST_F: { /* CH request: Active = low, Inactive = high (bimetallic room thermostat) */
            // CH request switch debouncing
            if ((GetFlag(p_sys, INPUT_FLAGS, CH_REQUEST_F)) == ((CH_RQ_PINP >> CH_RQ_PIN) & true)) {
            //if (((p_sys->input_flags >> CH_REQUEST_F) & true) == ((CH_RQ_PINP >> CH_RQ_PIN) & true)) {
                if (!(p_deb->ch_request_deb--)) {
                    p_deb->ch_request_deb = DLY_DEBOUNCE_CH_REQ;
                    if ((GetFlag(p_sys, INPUT_FLAGS, CH_REQUEST_F)) == ((CH_RQ_PINP >> CH_RQ_PIN) & true)) {
                    //if (((p_sys->input_flags >> CH_REQUEST_F) & true) == ((CH_RQ_PINP >> CH_RQ_PIN) & true)) {
                        p_sys->input_flags ^= (1 << CH_REQUEST_F);
                    }
                }
            }
            return ((p_sys->input_flags >> CH_REQUEST_F) & true);
        }
        case AIRFLOW_F: { /* Flue air flow sensor: Active = low, Inactive = high (flue air pressure switch) */
            // Airflow switch debouncing
            if ((GetFlag(p_sys, INPUT_FLAGS, AIRFLOW_F)) == ((AIRFLOW_PINP >> AIRFLOW_PIN) & true)) {
            //if (((p_sys->input_flags >> AIRFLOW_F) & true) == ((AIRFLOW_PINP >> AIRFLOW_PIN) & true)) {
                if (!(p_deb->airflow_deb--)) {
                    p_deb->airflow_deb = DLY_DEBOUNCE_AIRFLOW;
                    if ((GetFlag(p_sys, INPUT_FLAGS, AIRFLOW_F)) == ((AIRFLOW_PINP >> AIRFLOW_PIN) & true)) {
                    //if (((p_sys->input_flags >> AIRFLOW_F) & true) == ((AIRFLOW_PINP >> AIRFLOW_PIN) & true)) {
                        p_sys->input_flags ^= (1 << AIRFLOW_F);
                    }
                }
            }
            return ((p_sys->input_flags >> AIRFLOW_F) & true);
        }
        case FLAME_F: { /* Flame sensor: Active = high, Inactive = low. IT NEEDS EXTERNAL PULL-DOWN RESISTOR !!! */
            if ((FLAME_PINP >> FLAME_PIN) & true) {
                SetFlag(p_sys, INPUT_FLAGS, FLAME_F);
#if LED_UI_FOR_FLAME
                SetFlag(p_sys, OUTPUT_FLAGS, LED_UI_F);
#endif /* LED_UI_FOR_FLAME */
            } else {
                ClearFlag(p_sys, INPUT_FLAGS, FLAME_F);
#if LED_UI_FOR_FLAME
                ClearFlag(p_sys, OUTPUT_FLAGS, LED_UI_F);
#endif /* LED_UI_FOR_FLAME */
            }
            return ((p_sys->input_flags >> FLAME_F) & true);
        }
        case OVERHEAT_F: { /* Overheat thermostat: Active = high, Inactive = low. ACTIVE INDICATES OVERTEMPERATURE !!! */
            if ((OVERHEAT_PINP >> OVERHEAT_PIN) & true) {
                ClearFlag(p_sys, INPUT_FLAGS, OVERHEAT_F);
            } else {
                SetFlag(p_sys, INPUT_FLAGS, OVERHEAT_F);
                p_sys->input_flags |= (1 << OVERHEAT_F);
            }
            return ((p_sys->input_flags >> OVERHEAT_F) & true);
        }
    }
    if (ShowDashboard == true) {
        Dashboard(p_sys, false);
    }
    return 0;
}

//Function InitAnalogSensor: Sets the ADC hardware up and initializes the given sensor readout
void InitAnalogSensor(SysInfo *p_sys, AnalogInput analog_sensor) {
    ADMUX |= (1 << REFS0);                                /* reference voltage on AVCC */
    ACSR &= ~(1 << ACIE);                                 /* Clear analog comparator IRQ flag */
    ACSR = (1 << ACD);                                    /* Stop analog comparator */
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

// Function CheckAnalogSensor: Returns the ADC readout of a given analog sensor 
uint16_t CheckAnalogSensor(SysInfo *p_sys, AdcBuffers *p_buffer_pack, AnalogInput analog_sensor, bool ShowDashboard) {
    ADMUX = (0xF0 & ADMUX) | analog_sensor;
    ADCSRA |= (1 << ADSC);
    loop_until_bit_is_clear(ADCSRA, ADSC);
    switch (analog_sensor) {
        case DHW_TEMPERATURE: {
            p_buffer_pack->dhw_temp_adc_buffer.data[p_buffer_pack->dhw_temp_adc_buffer.ix++] = (ADC & 0x3FF);
            if (p_buffer_pack->dhw_temp_adc_buffer.ix >= BUFFER_LENGTH) {
                p_buffer_pack->dhw_temp_adc_buffer.ix = 0;
            }
            p_sys->dhw_temperature = AverageAdc(p_buffer_pack->dhw_temp_adc_buffer.data, BUFFER_LENGTH, 0, MEAN);
            break;
        }
        case CH_TEMPERATURE: {
            p_buffer_pack->ch_temp_adc_buffer.data[p_buffer_pack->ch_temp_adc_buffer.ix++] = (ADC & 0x3FF);
            if (p_buffer_pack->ch_temp_adc_buffer.ix >= BUFFER_LENGTH) {
                p_buffer_pack->ch_temp_adc_buffer.ix = 0;
            }
            p_sys->ch_temperature = AverageAdc(p_buffer_pack->ch_temp_adc_buffer.data, BUFFER_LENGTH, 0, MEAN);
            break;
        }
        case DHW_SETTING: {
            p_buffer_pack->dhw_set_adc_buffer.data[p_buffer_pack->dhw_set_adc_buffer.ix++] = (ADC & 0x3FF);
            if (p_buffer_pack->dhw_set_adc_buffer.ix >= BUFFER_LENGTH) {
                p_buffer_pack->dhw_set_adc_buffer.ix = 0;
            }
            p_sys->dhw_setting = AverageAdc(p_buffer_pack->dhw_set_adc_buffer.data, BUFFER_LENGTH, 0, MEAN);
            break;
        }
        case CH_SETTING: {
            p_buffer_pack->ch_set_adc_buffer.data[p_buffer_pack->ch_set_adc_buffer.ix++] = (ADC & 0x3FF);
            if (p_buffer_pack->ch_set_adc_buffer.ix >= BUFFER_LENGTH) {
                p_buffer_pack->ch_set_adc_buffer.ix = 0;
            }
            p_sys->ch_setting = AverageAdc(p_buffer_pack->ch_set_adc_buffer.data, BUFFER_LENGTH, 0, MEAN);
            break;
        }
        case SYSTEM_SETTING: {
            p_buffer_pack->sys_set_adc_buffer.data[p_buffer_pack->sys_set_adc_buffer.ix++] = (ADC & 0x3FF);
            if (p_buffer_pack->sys_set_adc_buffer.ix >= BUFFER_LENGTH) {
                p_buffer_pack->sys_set_adc_buffer.ix = 0;
            }
            p_sys->system_setting = AverageAdc(p_buffer_pack->sys_set_adc_buffer.data, BUFFER_LENGTH, 0, MEAN);
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

// Function InitActuator: Initializes a device actuator's output pin
void InitActuator(SysInfo *p_sys, OutputFlag device_flag) {
    switch (device_flag) {
        case EXHAUST_FAN_F: {
            FAN_DDR |= (1 << FAN_PIN);   /* Set exhaust fan pin as output */
            FAN_PORT &= ~(1 << FAN_PIN); /* Set exhaust fan pin low (inactive) */
            break;
        }
        case WATER_PUMP_F: {
            PUMP_DDR |= (1 << PUMP_PIN);   /* Set water pump pin as output */
            PUMP_PORT &= ~(1 << PUMP_PIN); /* Set water pump pin low (inactive) */
            break;
        }
        case SPARK_IGNITER_F: {
            SPARK_DDR |= (1 << SPARK_PIN);  /* Set spark igniter pin as output */
            SPARK_PORT |= (1 << SPARK_PIN); /* Set spark igniter pin high (inactive) */
            break;
        }
        case VALVE_S_F: {
            VALVE_S_DDR |= (1 << VALVE_S_PIN);   /* Set security valve pin as output */
            VALVE_S_PORT &= ~(1 << VALVE_S_PIN); /* Set security valve pin low (inactive) */
            break;
        }
        case VALVE_1_F: {
            VALVE_1_DDR |= (1 << VALVE_1_PIN);   /* Set security valve pin as output */
            VALVE_1_PORT &= ~(1 << VALVE_1_PIN); /* Set security valve pin low (inactive) */
            break;
        }
        case VALVE_2_F: {
            VALVE_2_DDR |= (1 << VALVE_2_PIN);   /* Set security valve pin as output */
            VALVE_2_PORT &= ~(1 << VALVE_2_PIN); /* Set security valve pin low (inactive) */
            break;
        }
        case VALVE_3_F: {
            VALVE_3_DDR |= (1 << VALVE_3_PIN);   /* Set security valve pin as output */
            VALVE_3_PORT &= ~(1 << VALVE_3_PIN); /* Set security valve pin low (inactive) */
            break;
        }
        case LED_UI_F: {
            LED_UI_DDR |= (1 << LED_UI_PIN);   /* Set LED UI pin as output */
            LED_UI_PORT &= ~(1 << LED_UI_PIN); /* Set LED UI pin low (inactive) */
            break;
        }
        default: {
            break;
        }
    }
    ClearFlag(p_sys, OUTPUT_FLAGS, device_flag); /* Clear actuator flags */
}

// Function ControlActuator: Turns an actuator pin and its associated flag on or off
void ControlActuator(SysInfo *p_sys, OutputFlag device_flag, HwSwitch command, bool ShowDashboard) {
    switch (device_flag) {
        case EXHAUST_FAN_F: {
            if (command == TURN_ON) {
                FAN_PORT |= (1 << FAN_PIN); /* Set exhaust fan pin high (active) */
            } else {
                FAN_PORT &= ~(1 << FAN_PIN); /* Set exhaust fan pin low (inactive) */
            }
            break;
        }
        case WATER_PUMP_F: {
            if (command == TURN_ON) {
                PUMP_PORT |= (1 << PUMP_PIN); /* Set water pump pin high (active) */
            } else {
                PUMP_PORT &= ~(1 << PUMP_PIN); /* Set water pump pin low (inactive) */
            }
            break;
        }
        case SPARK_IGNITER_F: {
            if (command == TURN_ON) {
                SPARK_PORT &= ~(1 << SPARK_PIN); /* Set spark igniter pin low (active) */
            } else {
                SPARK_PORT |= (1 << SPARK_PIN); /* Set spark igniter pin high (inactive) */
            }
            break;
        }
        case VALVE_S_F: {
            if (command == TURN_ON) {
                VALVE_S_PORT |= (1 << VALVE_S_PIN); /* Set security valve pin high (active) */
            } else {
                VALVE_S_PORT &= ~(1 << VALVE_S_PIN); /* Set security valve pin low (inactive) */
            }
            break;
        }
        case VALVE_1_F: {
            if (command == TURN_ON) {
                VALVE_1_PORT |= (1 << VALVE_1_PIN); /* Set valve 1 pin high (active) */
            } else {
                VALVE_1_PORT &= ~(1 << VALVE_1_PIN); /* Set valve 1 pin low (inactive) */
            }
            break;
        }
        case VALVE_2_F: {
            if (command == TURN_ON) {
                VALVE_2_PORT |= (1 << VALVE_2_PIN); /* Set valve 2 pin high (active) */
            } else {
                VALVE_2_PORT &= ~(1 << VALVE_2_PIN); /* Set valve 2 pin low (inactive) */
            }
            break;
        }
        case VALVE_3_F: {
            if (command == TURN_ON) {
                VALVE_3_PORT |= (1 << VALVE_3_PIN); /* Set valve 3 pin high (active) */
            } else {
                VALVE_3_PORT &= ~(1 << VALVE_3_PIN); /* Set valve 3 pin low (inactive) */
            }
            break;
        }
        case LED_UI_F: {
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
        if (!(GetFlag(p_sys, OUTPUT_FLAGS, device_flag))) {
            SetFlag(p_sys, OUTPUT_FLAGS, device_flag); /* Set actuator flags */
        }
    } else {
        if (GetFlag(p_sys, OUTPUT_FLAGS, device_flag)) {
            ClearFlag(p_sys, OUTPUT_FLAGS, device_flag); /* Clear actuator flags */
        }
    }
    if (ShowDashboard == true) {
        Dashboard(p_sys, false);
    }
}

// Function InitAdcBuffers: Initializes the ADC filtering buffers
void InitAdcBuffers(AdcBuffers *p_buffer_pack, uint8_t buffer_length) {
    p_buffer_pack->dhw_temp_adc_buffer.ix = 0;
    p_buffer_pack->ch_temp_adc_buffer.ix = 0;
    p_buffer_pack->dhw_set_adc_buffer.ix = 0;
    p_buffer_pack->ch_set_adc_buffer.ix = 0;
    p_buffer_pack->sys_set_adc_buffer.ix = 0;
    for (uint8_t i = 0; i < buffer_length; i++) {
        p_buffer_pack->dhw_temp_adc_buffer.data[i] = 0;
        p_buffer_pack->ch_temp_adc_buffer.data[i] = 0;
        p_buffer_pack->dhw_set_adc_buffer.data[i] = 0;
        p_buffer_pack->ch_set_adc_buffer.data[i] = 0;
        p_buffer_pack->sys_set_adc_buffer.data[i] = 0;
    }
}

// Function AverageAdc
uint16_t AverageAdc(uint16_t adc_buffer[], uint8_t buffer_len, uint8_t start, AverageType average_type) {
    uint16_t avg_value = 0;
    switch (average_type) {
        case MEAN: {
            for (uint8_t i = 0; i < buffer_len; i++) {
                avg_value += adc_buffer[i];
            }
            avg_value = avg_value / buffer_len;
            break;
        }
        case ROBUST: {
            uint16_t max, min;
            avg_value = max = min = adc_buffer[0];
            for (uint8_t i = 1; i < buffer_len; i++) {
                avg_value += adc_buffer[i];
                if (adc_buffer[i] > max) {
                    max = adc_buffer[i];
                } else if (adc_buffer[i] < min) {
                    min = adc_buffer[i];
                }
            }
            avg_value -= max;
            avg_value -= min;
            //avg_value = (avg_value >> 5);
            avg_value = avg_value / (buffer_len - 2);
            break;
        }
        case MOVING: {
            avg_value += adc_buffer[start];
            if (start == (buffer_len - 1)) {
                avg_value -= adc_buffer[0];
            } else {
                avg_value -= adc_buffer[start + 1];
            }
            break;
        }
        default: {
            break;
        }
    }
    return avg_value;
}

// Function GetHeatLevel: Returns a heat level index from a given potentiometer readout and range-intervals number
uint8_t GetHeatLevel(int16_t pot_adc_value, uint8_t knob_steps) {
    uint8_t heat_level = 0;
    for (heat_level = 0; (pot_adc_value < (ADC_MAX - ((ADC_MAX / knob_steps) * (heat_level + 1)))); heat_level++)
        ;
    if (heat_level >= knob_steps) {
        heat_level = --knob_steps;
    }
    return heat_level;
}

// Function OpenHeatValve: Opens a given heat valve exclusively, closing all the others
void OpenHeatValve(SysInfo *p_sys, HeatValve valve_to_open) {
    uint8_t modulator_valve_count = GAS_MODULATOR_VALVES;
    for (uint8_t valve = 0; valve < modulator_valve_count; valve++) {
        if (valve == valve_to_open - 1) {
            if (GetFlag(p_sys, OUTPUT_FLAGS, p_sys->gas_modulator[valve].valve_flag) == false) {
                SetFlag(p_sys, OUTPUT_FLAGS, p_sys->gas_modulator[valve].valve_flag);
            }
        } else {
            if (GetFlag(p_sys, OUTPUT_FLAGS, p_sys->gas_modulator[valve].valve_flag)) {
                ClearFlag(p_sys, OUTPUT_FLAGS, p_sys->gas_modulator[valve].valve_flag);
            }
        }
    }
}

// Function GasOff: Closes all heat valves and the security valve, turns the spark igniter and exhaust fan off
void GasOff(SysInfo *p_sys) {
    // Turn spark igniter off
    ClearFlag(p_sys, OUTPUT_FLAGS, SPARK_IGNITER_F);
    // Close gas valve 3
    ClearFlag(p_sys, OUTPUT_FLAGS, VALVE_3_F);
    // Close gas valve 2
    ClearFlag(p_sys, OUTPUT_FLAGS, VALVE_2_F);
    // Close gas valve 1
    ClearFlag(p_sys, OUTPUT_FLAGS, VALVE_1_F);
    // Close gas security valve
    ClearFlag(p_sys, OUTPUT_FLAGS, VALVE_S_F);
    // Turn exhaust fan off
    ClearFlag(p_sys, OUTPUT_FLAGS, EXHAUST_FAN_F);
}
