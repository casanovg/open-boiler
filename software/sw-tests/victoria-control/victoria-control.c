// ---------------------------------------------
// Test 04 - 2019-11-23 - Gustavo Casanova
// .............................................
// Heat modulation algorithm - new
// ---------------------------------------------

#include "include/victoria-control.h"

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
    // Set timer for 1 ms tick rate
    SetTickTimer();

    unsigned long fofo = GetMilliseconds();
    fofo++;
    
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
    // GasValve gas_valve[] = {
        // {VALVE_1, 7000, 0.87, 0},
        // {VALVE_2, 12000, 1.46, 0},
        // {VALVE_3, 20000, 2.39, 0}};

    // static const uint16_t cycle_time = 50000;
    // //uint8_t cycle_slots = 6;
    // bool cycle_in_progress = 0;
    // uint8_t system_valves = 3;
    // //uint8_t system_valves = (sizeof(gas_valve) / sizeof(gas_valve[0]));
    // //uint8_t dhw_heat_level = 7; /* This level is determined by the CH temperature potentiometer */
    // //uint8_t dhw_heat_level = GetHeatLevel(p_system->ch_setting, DHW_SETTING_STEPS);
    // uint8_t current_valve = 0;
    // uint32_t valve_open_timer = 0;
    // NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW

    // ADC buffers initialization
    AdcBuffers buffer_pack;
    AdcBuffers *p_buffer_pack = &buffer_pack;
    InitAdcBuffers(p_buffer_pack, BUFFER_LENGTH);

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

    // Pre-load analog sensor values
    for (uint8_t i = 0; i < BUFFER_LENGTH; i++) {
        for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
            CheckAnalogSensor(p_system, p_buffer_pack, analog_sensor, false);
        }
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

        //Dashboard(p_system, false);

        // Test outputs
        if (!(GetFlag(p_system, OUTPUT_FLAGS, VALVE_S))) {
            SetFlag(p_system, OUTPUT_FLAGS, VALVE_S);
            _delay_ms(1000);
        }

        if (!(GetFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER))) {
            SetFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER);
            _delay_ms(1000);
        }
        //if (!(p_system->output_flags >> VALVE_1) & true) {
        //GasOff(p_system);
        SetFlag(p_system, OUTPUT_FLAGS, VALVE_1);
        ClearFlag(p_system, OUTPUT_FLAGS, VALVE_3);
        _delay_ms(1000);
        //}
        //if (!(p_system->output_flags >> VALVE_2) & true) {
        //GasOff(p_system);
        SetFlag(p_system, OUTPUT_FLAGS, VALVE_2);
        ClearFlag(p_system, OUTPUT_FLAGS, VALVE_1);
        _delay_ms(1000);
        //}
        //if (!(p_system->output_flags >> VALVE_3) & true) {
        //GasOff(p_system);
        SetFlag(p_system, OUTPUT_FLAGS, VALVE_3);
        ClearFlag(p_system, OUTPUT_FLAGS, VALVE_2);
        _delay_ms(1000);
        //}

    } /* Main loop end */

    return 0;
}

// Function SetTickTimer
void SetTickTimer(void) {
    // Set prescaler factor 64
    TCCR0B |= (1 << CS00);
    TCCR0B |= (1 << CS01);
    // Enable timer 0 overflow interrupt
    TIMSK0 |= (1 << TOIE0);
}

// Timer 0 overflow interrupt service routine
ISR(TIMER0_OVF_vect) {

	// copy these to local variables so they can be stored in registers
	// (volatile variables must be read from memory on every access)
	unsigned long m = timer0_milliseconds;
	unsigned char f = timer0_fractions;

	m += MILLIS_INC;
	f += FRACT_INC;
	if (f >= FRACT_MAX) {
		f -= FRACT_MAX;
		m += 1;
	}

	timer0_fractions = f;
	timer0_milliseconds = m;
	timer0_overflow_count++;
}

// Function GetMilliseconds
unsigned long GetMilliseconds(void) {
	unsigned long m;
	uint8_t oldSREG = SREG;

	// disable interrupts while we read timer0_milliseconds or we might get an
	// inconsistent value (e.g. in the middle of a write to timer0_millis)
	cli();
	m = timer0_milliseconds;
	SREG = oldSREG;

	return m;
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
    wdt_enable(WDTO_15MS); /* Restart by activating the watchdog timer */
    for (;;) {
    };
}

// Function InitAnalogSensor
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

// Function InitAdcBuffers
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

// Function FIR filter
uint16_t FilterFir(uint16_t adc_buffer[], uint8_t buffer_len, uint8_t start) {
    unsigned long int aux = 0;
    uint16_t result;
    for (uint8_t i = 0; i < FIR_LEN; i++) {
        aux += (unsigned long)(adc_buffer[start++]) * (unsigned long)(fir_table[i]);
        if (start == buffer_len) {
            start = 0;
        }
    }
    result = ((unsigned long int)(aux)) / ((unsigned long int)(FIR_SUM));
    return result;
}

// Function IIR filter
uint16_t FilterIir(uint16_t value) {
    static uint16_t Y;
    Y = (64 - IR_VAL) * value + IR_VAL * Y;
    Y = (Y >> 6);
    return Y;
}

// Function CalculateNtcTemperature
int GetNtcTemperature(uint16_t ntc_adc_value, int temp_offset, int temp_delta) {
    int aux;
    uint16_t min, max;
    uint8_t i;
    // Search the table interval where the ADC value is located
    for (i = 0; (i < NTC_VALUES) && (ntc_adc_value < (ntc_adc_table[i])); i++)
        ;
    if ((i == 0) || (i == NTC_VALUES)) {  // If there is not located, return an error
        return -32767;
    }
    max = ntc_adc_table[i - 1];                 //Buscamos el valor más alto del intervalo
    min = ntc_adc_table[i];                     //y el más bajoa
    aux = (max - ntc_adc_value) * temp_delta;   //hacemos el primer paso de la interpolación
    aux = aux / (max - min);                    //y el segundo paso
    aux += (i - 1) * temp_delta + temp_offset;  //y añadimos el offset del resultado
    return aux;
}

// Function GetNtcTempTenths
int GetNtcTempTenths(uint16_t ntc_adc_value, int temp_offset, int temp_delta) {
    int aux;
    uint16_t min, max;
    uint8_t i;
    // Search the table interval where the ADC value is located
    for (i = 0; (i < NTC_VALUES) && (ntc_adc_value < (ntc_adc_table[i])); i++)
        ;
    if ((i == 0) || (i == NTC_VALUES)) {  // If there is not located, return an error
        return -32767;
    }
    //printf("ADC table entry (%d) = %d\n\r", i, ntc_adc_table[i]);
    max = ntc_adc_table[i - 1];                 //Buscamos el valor más alto del intervalo
    min = ntc_adc_table[i];                     //y el más bajoa
    aux = (max - ntc_adc_value) * temp_delta;   //hacemos el primer paso de la interpolación
    aux = aux / (max - min);                    //y el segundo paso
    aux += (i - 1) * temp_delta + temp_offset;  //y añadimos el offset del resultado
    return aux;
}

// Function GetNtctempDegrees
float GetNtcTempDegrees(uint16_t ntc_adc_value, int temp_offset, int temp_delta) {
    int aux;
    uint16_t min, max;
    uint8_t i;
    // Search the table interval where the ADC value is located
    for (i = 0; (i < NTC_VALUES) && (ntc_adc_value < (ntc_adc_table[i])); i++)
        ;
    if ((i == 0) || (i == NTC_VALUES)) {  // If there is not located, return an error
        return -32767.0;
    }
    max = ntc_adc_table[i - 1];                 //Buscamos el valor más alto del intervalo
    min = ntc_adc_table[i];                     //y el más bajoa
    aux = (max - ntc_adc_value) * temp_delta;   //hacemos el primer paso de la interpolación
    aux = aux / (max - min);                    //y el segundo paso
    aux += (i - 1) * temp_delta + temp_offset;  //y añadimos el offset del resultado
    return ((float)aux / 10);
}

// Function GetHeatLevel
uint8_t GetHeatLevel(int16_t pot_adc_value, uint8_t knob_steps) {
    uint8_t heat_level = 0;
    for (heat_level = 0; (pot_adc_value < (ADC_MAX - ((ADC_MAX / knob_steps) * (heat_level + 1)))); heat_level++)
        ;
    if (heat_level >= knob_steps) {
        heat_level = --knob_steps;
    }
    return heat_level;
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
            //sprintf(str, "%07lu", data);
            sprintf(str, "%0lu", (unsigned long int)data);
            break;
        }
        case DIGITS_FREE:
        default: {
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
        SerialTxChr(32); /* Space (_) */
        SerialTxNum(GetNtcTemperature(p_sys->dhw_temperature, TO_CELSIUS, DT_CELSIUS), DIGITS_3);

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // CH temperature
        SerialTxStr(str_lit_14);
        SerialTxNum(p_sys->ch_temperature, DIGITS_4);
        SerialTxChr(32); /* Space (_) */
        SerialTxNum(GetNtcTemperature(p_sys->ch_temperature, TO_CELSIUS, DT_CELSIUS), DIGITS_3);

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
        SerialTxChr(32); /* Space (_) */
        //===SerialTxNum(GetHeatLevel(p_sys->dhw_setting, DHW_SETTING_STEPS), DIGITS_2);
        SerialTxNum(p_sys->dhw_heat_level, DIGITS_2);

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        SerialTxStr(str_lit_16);
        SerialTxNum(p_sys->ch_setting, DIGITS_4);
        SerialTxChr(32); /* Space (_) */
        SerialTxNum(GetHeatLevel(p_sys->ch_setting, DHW_SETTING_STEPS), DIGITS_2);

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
