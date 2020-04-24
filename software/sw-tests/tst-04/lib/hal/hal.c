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

# include "hal.h"

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
                    if (((p_sys->input_flags >> CH_REQUEST) & true) == ((CH_RQ_PINP >> CH_RQ_PIN) & true)) {
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
                    if (((p_sys->input_flags >> AIRFLOW) & true) == ((AIRFLOW_PINP >> AIRFLOW_PIN) & true)) {
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

//Function InitAnalogSensor
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

// Function CheckAnalogSensor
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
