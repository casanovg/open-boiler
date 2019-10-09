/*
 *  Open-Boiler Control
 *  ....................................
 *  File: victoria-control.c
 *  ....................................
 *  v0.5 / 2019-08-11 / ATmega328
 *  ....................................
 *  Gustavo Casanova / Nicebots
 */

#include "victoria-control.h"

// Prototypes
void LedUiOn(void);
void LedUiOff(void);

// Main function
int main(void) {
    /*  ___________________
       |                   | 
       |    Setup Block    |
       |___________________|
    */

    //System state initialization
    SysInfo sys_info;
    SysInfo *p_system = &sys_info;
    p_system->system_state = OFF;
    p_system->error = ERROR_000;
    p_system->input_flags = 0;
    p_system->output_flags = 0;
    p_system->last_displayed_iflags = 0;
    p_system->last_displayed_oflags = 0;

    int l_delay = DLY_L_OFF;
    uint8_t s_delay = DLY_S_OFF;

    // Initialize USART for serial communications (38400, N, 8, 1)
    SerialInit();

    // Initialize actuator controls
    for (OutputFlag device = EXHAUST_FAN; device <= LED_UI; device++) {
        InitActuator(p_system, device);
    }

    // Initialize digital sensor flags
    for (InputFlag digital_sensor = DHW_REQUEST; digital_sensor <= OVERHEAT; digital_sensor++) {
        InitDigitalSensor(p_system, digital_sensor);
    }

    // Initialize analog sensor inputs
    for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
        InitAnalogSensor(p_system, analog_sensor);
    }

    Dashboard(p_system, false);

    /*  ___________________
       |                   | 
       |     Main Loop     |
       |___________________|
    */
    for (;;) {
        // Update digital input sensors status
        for (InputFlag digital_sensor = DHW_REQUEST; digital_sensor <= OVERHEAT; digital_sensor++) {
            CheckDigitalSensor(p_system, digital_sensor, true);
        }

        // Update analog input sensors status
        for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
            CheckAnalogSensor(p_system, analog_sensor, true);
        }
        // Display updated status
        //Dashboard(p_system, false);
        // System FSM
        switch (p_system->system_state) {
            // .......................
            // . System state -> OFF  .
            // .......................
            case OFF: {
                // Turn all actuators off
                for (OutputFlag device = EXHAUST_FAN; device <= LED_UI; device++) {
                    ControlActuator(p_system, device, TURN_OFF, true);
                }
                Dashboard(p_system, true);
                // Check status of input sensors (loops 255 times)
                for (uint8_t i = 0; i < 255; i++) {
                    // Verify that the flame sensor is off at this point, otherwise, its an anomaly
                    if ((p_system->input_flags >> FLAME) & true) {
                        p_system->error = ERROR_001;
                        // Next state -> ERROR
                        //p_system->system_state = ERROR;
                    }
#if !(OVERHEAT_OVERRIDE)
                    // Verify that the overheat thermostat is not open at this point, otherwise, its an anomaly
                    if ((p_system->input_flags >> OVERHEAT) & true) {
                        p_system->error = ERROR_002;
                        // Next state -> ERROR
                        //p_system->system_state = ERROR;
                    }
#endif /* OVERHEAT_OVERRIDE */
#if !(AIRFLOW_OVERRIDE)
                    // Verify that the airflow sensor is off at this point, otherwise, its an anomaly
                    if ((p_system->input_flags >> AIRFLOW) & true) {
                        p_system->error = ERROR_003;
                        // Next state -> ERROR
                        //p_system->system_state = ERROR;
                    }
#endif /* AIRFLOW_OVERRIDE */
                    _delay_ms(15);
                }
#if !(AIRFLOW_OVERRIDE)
                // Exhaust fan test
                if (!(l_delay--) {
                    l_delay = DLY_L_OFF;
                    
                }
#endif /* AIRFLOW_OVERRIDE */
                p_system->last_displayed_iflags = 0xFF; /* Force a flag change to display dashboard */
                if (p_system->error) {
                    p_system->system_state = ERROR;
                } else {
                    // If everything is OK, setup delays and go ahead to "ready" state
                    l_delay = DLY_L_READY;
                    p_system->system_state = READY;
                }
                break;
            }
            // .........................
            // . System state -> READY  .
            // .........................
            case READY: {
                if (!(l_delay--)) {
                    l_delay = DLY_L_READY;
                    Dashboard(p_system, true);
                }
                break;
            }
            // ............................
            // . System state -> IGNITING  .
            // ............................
            case IGNITING: {
                //Dashboard(p_system, true);
                break;
            }
            // ...............................
            // . System state -> ON_DHW_DUTY  .
            // ...............................
            case ON_DHW_DUTY: {
                for (OutputFlag device = EXHAUST_FAN; device <= LED_UI; device++) {
                    ControlActuator(p_system, device, TURN_ON, true);
                    //_delay_ms(250);
                }
                //_delay_ms(250);
                p_system->system_state = READY;
                break;
            }
            // ...............................
            // . System state -> ON_CH_DUTY  .
            // ...............................
            case ON_CH_DUTY: {
                //Dashboard(p_system, true);
                break;
            }
            // .........................
            // . System state -> ERROR  .
            // .........................
            case ERROR: {
                // Turn all actuators off
                for (OutputFlag device = EXHAUST_FAN; device <= LED_UI; device++) {
                    ControlActuator(p_system, device, TURN_OFF, true);
                }
                uint8_t error_loops = 50;
                while (error_loops--) {
                    ControlActuator(p_system, LED_UI, TURN_ON, true);
                    SerialTxStr(str_crlf);
                    SerialTxStr(str_error);
                    SerialTxNum(p_system->error, DIGITS_3);
                    _delay_ms(500);
                    ControlActuator(p_system, LED_UI, TURN_OFF, false);
                    _delay_ms(500);
                }
                // Next state -> OFF (reset error and resume service)
                p_system->error = ERROR_000;
                p_system->system_state = OFF;
                break;
            }
            // ...........
            // . Default  .
            // ...........
            default: {
                //
                break;
            }
        }
    }
    return 0;
}

// Function InitAnalogSensor
void InitAnalogSensor(SysInfo *p_sys, AnalogInput analog_sensor) {
    ADMUX |= (1 << REFS0);                                /* reference voltage on AVCC */
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
    ADMUX = (0xf0 & ADMUX) | analog_sensor;
    ADCSRA |= (1 << ADSC);
    loop_until_bit_is_clear(ADCSRA, ADSC);
    switch (analog_sensor) {
        case DHW_TEMPERATURE: {
            p_sys->dhw_temperature = ADC;
            break;
        }
        case CH_TEMPERATURE: {
            p_sys->ch_temperature = ADC;
            break;
        }
        case DHW_SETTING: {
            p_sys->dhw_setting = ADC;
            break;
        }
        case CH_SETTING: {
            p_sys->ch_setting = ADC;
            break;
        }
        case SYSTEM_SETTING: {
            p_sys->system_setting = ADC;
            break;
        }
        default: {
            break;
        }
    }
    if (ShowDashboard == true) {
        Dashboard(p_sys, false);
    }
    return (ADC);
}

// Function InitFlags
void InitFlags(SysInfo *p_sys, FlagsByte flags_byte) {
    switch (flags_byte) {
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
void SetFlag(SysInfo *p_sys, FlagsByte flags_byte, uint8_t flag_position, bool control_hw) {
    switch (flags_byte) {
        case INPUT_FLAGS: {
            p_sys->input_flags |= (1 << flag_position);
            break;
        }
        case OUTPUT_FLAGS: {
            // Warning! This function activates actual hardware!
            if (control_hw) {
                ControlActuator(p_sys, flag_position, TURN_ON, true);
            }
            p_sys->output_flags &= ~(1 << flag_position);
            break;
        }
        default: {
            break;
        }
    }
}

void print_trace(void) {
    void *array[10];
    size_t size;
    char **strings;
    size_t i;
    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);
    printf("Obtained %zd stack frames.\n", size);
    for (i = 0; i < size; i++) {
        printf("%s\n", strings[i]);
    }
    free(strings);
}

// Function ClearFlag
void ClearFlag(SysInfo *p_sys, FlagsByte flags_byte, uint8_t flag_position, bool control_hw) {
    switch (flags_byte) {
        case INPUT_FLAGS: {
            p_sys->input_flags &= ~(1 << flag_position);
            break;
        }
        case OUTPUT_FLAGS: {
            // Warning! This function deactivates actual hardware!
            if (control_hw) {
                ControlActuator(p_sys, flag_position, TURN_OFF, true);
            }
            p_sys->output_flags &= ~(1 << flag_position);
            break;
        }
        default: {
            break;
        }
    }
}

// Function GetFlag
bool GetFlag(SysInfo *p_sys, FlagsByte flags_byte, uint8_t flag_position) {
    switch (flags_byte) {
        case INPUT_FLAGS: {
            return ((p_sys->input_flags >> flag_position) & true);
            break;
        }
        case OUTPUT_FLAGS: {
            return ((p_sys->output_flags >> flag_position) & true);
            break;
        }
        default: {
            break;
        }
    }
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
    ClearFlag(p_sys, INPUT_FLAGS, digital_sensor, false);
}

// Function CheckDigitalSensor
bool CheckDigitalSensor(SysInfo *p_sys, InputFlag digital_sensor, bool ShowDashboard) {
    switch (digital_sensor) {
        case DHW_REQUEST: { /* DHW request: Active = low, Inactive = high */
            if ((DHW_RQ_PINP >> DHW_RQ_PIN) & true) {
                ClearFlag(p_sys, INPUT_FLAGS, DHW_REQUEST, false);
            } else {
                SetFlag(p_sys, INPUT_FLAGS, DHW_REQUEST, false);
            }
            return ((p_sys->input_flags >> DHW_REQUEST) & true);
        }
        case CH_REQUEST: { /* CH request: Active = low, Inactive = high */
            if ((CH_RQ_PINP >> CH_RQ_PIN) & true) {
                ClearFlag(p_sys, INPUT_FLAGS, CH_REQUEST, false);
            } else {
                SetFlag(p_sys, INPUT_FLAGS, CH_REQUEST, false);
            }
            return ((p_sys->input_flags >> CH_REQUEST) & true);
        }
        case AIRFLOW: { /* Air flow sensor: Active = low, Inactive = high */
            if ((AIRFLOW_PINP >> AIRFLOW_PIN) & true) {
                ClearFlag(p_sys, INPUT_FLAGS, AIRFLOW, false);
            } else {
                SetFlag(p_sys, INPUT_FLAGS, AIRFLOW, false);
            }
            return ((p_sys->input_flags >> AIRFLOW) & true);
        }
        case FLAME: { /* Flame sensor: Active = high, Inactive = low. IT NEEDS EXTERNAL PULL-DOWN RESISTOR !!! */
            if ((FLAME_PINP >> FLAME_PIN) & true) {
                SetFlag(p_sys, INPUT_FLAGS, FLAME, false);
            } else {
                ClearFlag(p_sys, INPUT_FLAGS, FLAME, false);
            }
            return ((p_sys->input_flags >> FLAME) & true);
        }
        case OVERHEAT: { /* Overheat thermostat: Active = low, Inactive = high. ACTIVE MEANS OVERTEMPERATURE !!! */
            if ((OVERHEAT_PINP >> OVERHEAT_PIN) & true) {
                ClearFlag(p_sys, INPUT_FLAGS, OVERHEAT, false);
            } else {
                SetFlag(p_sys, INPUT_FLAGS, OVERHEAT, false);
            }
            return ((p_sys->input_flags >> OVERHEAT) & true);
        }
    }
    if (ShowDashboard == true) {
        Dashboard(p_sys, false);
    }
}

// // Function CheckDigitalSensor (OK!
// bool CheckDigitalSensor(SysInfo *p_sys, InputFlag digital_sensor, bool ShowDashboard) {
//     switch (digital_sensor) {
//         case DHW_REQUEST: {
//             if ((DHW_RQ_PINP >> DHW_RQ_PIN) & true) {
//                 p_sys->input_flags &= ~(1 << DHW_REQUEST);
//             } else {
//                 p_sys->input_flags |= (1 << DHW_REQUEST);
//             }
//             return ((p_sys->input_flags >> DHW_REQUEST) & true);
//         }
//         case CH_REQUEST: {
//             if ((CH_RQ_PINP >> CH_RQ_PIN) & true) {
//                 p_sys->input_flags &= ~(1 << CH_REQUEST);
//             } else {
//                 p_sys->input_flags |= (1 << CH_REQUEST);
//             }
//             return ((p_sys->input_flags >> CH_REQUEST) & true);
//         }
//         case AIRFLOW: {
//             if ((AIRFLOW_PINP >> AIRFLOW_PIN) & true) {
//                 p_sys->input_flags &= ~(1 << AIRFLOW);
//             } else {
//                 p_sys->input_flags |= (1 << AIRFLOW);
//             }
//             return ((p_sys->input_flags >> AIRFLOW) & true);
//         }
//         case FLAME: { /* IT NEEDS EXTERNAL PULL-DOWN RESISTOR !!! */
//             if ((FLAME_PINP >> FLAME_PIN) & true) {
//                 p_sys->input_flags |= (1 << FLAME);
//             } else {
//                 p_sys->input_flags &= ~(1 << FLAME);
//             }
//             return ((p_sys->input_flags >> FLAME) & true);
//         }
//         case OVERHEAT: { /* OVERHEAD CHECKING DISABLED !!! UNCOMMENT FOR PROD */
//             if ((OVERHEAT_PINP >> OVERHEAT_PIN) & true) {
//                 p_sys->input_flags &= ~(1 << OVERHEAT);
//             } else {
//                 p_sys->input_flags |= (1 << OVERHEAT);
//             }
//             //p_sys->input_flags &= ~(1 << OVERHEAT); /* FORCED TO NOT-OVERHEAT !!! COMMENT FOR PROD */
//             return ((p_sys->input_flags >> OVERHEAT) & true);
//         }
//     }
//     if (ShowDashboard == true) {
//         Dashboard(p_sys, false);
//     }
// }

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
    ClearFlag(p_sys, OUTPUT_FLAGS, device, false); /* Clear actuator flags */
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
    // Keep output flags in sync with hardware controls, just in case this function isn't called through set/clear flag functions
    if (command == TURN_ON) {
        SetFlag(p_sys, OUTPUT_FLAGS, device, false); /* Set actuator flags */
    } else {
        ClearFlag(p_sys, OUTPUT_FLAGS, device, false); /* Clear actuator flags */
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
void SerialTxNum(uint16_t data, DigitLength digits) {
#define DATA_LNG 5
    char str[DATA_LNG] = {0};
    switch (digits) {
        case DIGITS_1: {
            sprintf(str, "%01d", data);
            break;
        }
        case DIGITS_2: {
            sprintf(str, "%02d", data);
            break;
        }
        case DIGITS_3: {
            sprintf(str, "%03d", data);
            break;
        }
        case DIGITS_4: {
            sprintf(str, "%04d", data);
            break;
        }
        case DIGITS_5: {
            sprintf(str, "%05d", data);
            break;
        }
        case DIGITS_FREE:
        defaut : {
            sprintf(str, "%d", data);
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
void SerialTxStr(char *ptr_string) {
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
            case ON_DHW_DUTY: {
                SerialTxStr(str_mode_30);
                break;
            }
            case ON_CH_DUTY: {
                SerialTxStr(str_mode_40);
                break;
            }
            case ERROR: {
                SerialTxStr(str_mode_100);
                break;
            }
        }

        //SerialTxStr(str_crlf);                  /* CR + new line */

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
        //SerialTxStr("255");

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // DHW temperature
        SerialTxStr(str_lit_13);
        SerialTxNum(p_sys->dhw_temperature, DIGITS_4);
        //SerialTxStr("0");

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // CH temperature
        SerialTxStr(str_lit_14);
        SerialTxNum(p_sys->ch_temperature, DIGITS_4);
        //SerialTxStr("1023");

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

        //SerialTxChr(32);                        /* Space (_) */
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

        //SerialTxChr(32);                        /* Space (_) */
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

        //SerialTxChr(32);                        /* Space (_) */
        //SerialTxChr(32);                        /* Space (_) */
        SerialTxChr(32);     /* Space (_) */
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
        //SerialTxStr("255");

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

// -----------------------------------------------------------------------------------------

// // Prototypes
// void SerialInit(void);
// unsigned char SerialRXChr(void);
// void SerialTXChr(unsigned char data);
// void SerialTXStr(char* ptr_string);
// void SerialTxNum(uint16_t data);
// void ShowError(int delay_millis);
// void ClrScr(void);
// void AllOff(void);

// // Globals
// /* str_string[] is in fact an array but when we put the text between the " " symbols the compiler threats
//  * it as a string and automatically puts the null termination character in the end of the text.
//  */
// const char __flash str_header_01[] = {"\n\rOpen-Boiler v0.4 (2019-08-08)\n\r"};
// const char __flash str_header_02[] = {"\n\rHi Juan, Sandra & Gustavo!\n\r"};
// const char __flash str_header_03[] = {"\n\r--> System ready\n\n\r"};
// const char __flash str_flame_01[] = {"\n\r>>> Flame detected\n\r"};
// const char __flash str_flame_02[] = {"\n\r>>> No flame detected\n\r"};
// const char __flash str_dhw_rq_01[] = {"\n\r>>> Domestic Hot Water requested!\n\r"};
// const char __flash str_dhw_rq_02[] = {"\n\r>>> End of DHW request!\n\r"};
// const char __flash str_fan_01[] = {"\n\r--> Turning fan on\n\r"};
// const char __flash str_fan_02[] = {"\n\r--> Turning fan off\n\r"};
// const char __flash str_valve_s_01[] = {"\n\r--> Security valve open\n\r"};
// const char __flash str_valve_s_02[] = {"\n\r--> Security valve closed\n\r"};
// const char __flash str_valve_1_01[] = {"\n\r--> Gas valve 1 open\n\r"};
// const char __flash str_valve_1_02[] = {"\n\r--> Gas valve 1 closed\n\r"};
// const char __flash str_valve_2_01[] = {"\n\r--> Gas valve 2 open\n\r"};
// const char __flash str_valve_2_02[] = {"\n\r--> Gas valve 2 closed\n\r"};
// const char __flash str_spark_01[] = {"\n\r--> Spark igniter on\n\r"};
// const char __flash str_spark_02[] = {"\n\r--> Spark igniter off\n\r"};
// const char __flash str_dhw_duty_01[] = {"\n\r--> Boiler on DHW duty!\n\r"};
// const char __flash str_dhw_duty_02[] = {"\n\r--> End of boiler DHW duty\n\r"};
// const char __flash str_gas_off[] = {"\n\r--> Closing all gas valves\n\r"};
// const char __flash str_ignition_err[] = {"Ignition error! "};
// const char __flash str_parameter[] = {"Parameter: "};
// const char __flash crlf[] = {"\r\n"};

// long spark_delay = SPARK_DELAY;
// long valve_1_delay = VALVE_1_DELAY;
// bool flame = false;
// bool sparks = false;
// bool fan = false;
// bool valve_s = false;
// bool valve_1 = false;
// bool valve_2 = false;
// bool valve_3 = false;
// bool on_duty = false;
// bool dhw_req = false;

// // Main function
// int main(void) {
//     /*  ___________________
//        |                   |
//        |    Setup Block    |
//        |___________________|
//     */
//     LED_UI_DDR |= (1 << LED_UI_PIN);   /* Set LED UI pin as output */
//     LED_UI_PORT &= ~(1 << LED_UI_PIN); /* Set LED UI pin low (inactive) */

//     DHW_RQ_DDR &= ~(1 << DHW_RQ_PIN); /* Set DHW request pin as input */
//     DHW_RQ_PORT |= (1 << DHW_RQ_PIN); /* Activate pull-up resistor on this pin */

//     FLAME_DDR &= ~(1 << FLAME_PIN);  /* Set Flame detector pin as input */
//     FLAME_PORT &= ~(1 << FLAME_PIN); /* Deactivate pull-up resistor on this pin */

//     FAN_DDR |= (1 << FAN_PIN);   /* Set exhaust fan pin as output */
//     FAN_PORT &= ~(1 << FAN_PIN); /* Set exhaust fan pin low (inactive) */

//     SPARK_DDR |= (1 << SPARK_PIN);  /* Set spark igniter pin as output */
//     SPARK_PORT |= (1 << SPARK_PIN); /* Set spark igniter pin high (inactive) */

//     VALVE_S_DDR |= (1 << VALVE_S_PIN);   /* Set security valve pin as output */
//     VALVE_S_PORT &= ~(1 << VALVE_S_PIN); /* Set security valve pin low (inactive) */

//     VALVE_1_DDR |= (1 << VALVE_1_PIN);   /* Set valve 1 output pin as output */
//     VALVE_1_PORT &= ~(1 << VALVE_1_PIN); /* Set valve 1 output pin low (inactive) */

//     VALVE_2_DDR |= (1 << VALVE_2_PIN);   /* Set valve 2 output pin as output */
//     VALVE_2_PORT &= ~(1 << VALVE_2_PIN); /* Set valve 2 output pin low (inactive) */

//     VALVE_3_DDR |= (1 << VALVE_3_PIN);   /* Set valve 3 output pin as output */
//     VALVE_3_PORT &= ~(1 << VALVE_3_PIN); /* Set valve 3 output pin low (inactive) */

//     SerialInit();  //Call the USART initialization code

//     _delay_ms(1000);
//     ClrScr();
//     SerialTXStr(str_header_01);
//     SerialTXStr(str_header_02);
//     SerialTXStr(str_header_03);

//     // for (int z = 0; z < 32768; z++ ) {
//     //     _delay_ms(1000);
//     //     ClrScr();
//     //     SerialTXStr(str_header_01);
//     //     SerialTXStr(str_header_02);
//     //     SerialTXStr(str_header_03);
//     //     //_delay_ms(500);
//     //     SerialTXStr(str_parameter);
//     //     SerialTxNum(z);
//     //     SerialTXStr(crlf);
//     // }

//     /*  ___________________
//        |                   |
//        |     Main Loop     |
//        |___________________|
//     */
//     for (;;) {
//         // Check flame detector state
//         if (FLAME_PINP & (1 << FLAME_PIN)) {
//             if (flame == false) {
//                 SerialTXStr(str_flame_01);
//                 LED_UI_PORT |= (1 << LED_UI_PIN);
//                 flame = true;
//             }
//         } else {
//             if (flame == true) {
//                 SerialTXStr(str_flame_02);
//                 LED_UI_PORT &= ~(1 << LED_UI_PIN);
//                 flame = false;
//             }
//         }

//         // Check domestic hot water request detector
//         if ((DHW_RQ_PINP & (1 << DHW_RQ_PIN)) == 0) {
//             if (dhw_req == false) {
//                 dhw_req = true;
//                 SerialTXStr(str_dhw_rq_01);
//             }

//             if (flame == false) {
//                 if (fan == false) {
//                     FAN_PORT |= (1 << FAN_PIN);
//                     SerialTXStr(str_fan_01);
//                     fan = true;
//                     _delay_ms(2000);
//                 }

//                 if (valve_s == false) {
//                     VALVE_S_PORT |= (1 << VALVE_S_PIN);
//                     SerialTXStr(str_valve_s_01);
//                     valve_s = true;
//                     _delay_ms(500);
//                 }

//                 if (valve_1 == false) {
//                     VALVE_1_PORT |= (1 << VALVE_1_PIN);
//                     SerialTXStr(str_valve_1_01);
//                     valve_1 = true;
//                     _delay_ms(500);
//                 }

//                 if (sparks == false) {
//                     SPARK_PORT &= ~(1 << SPARK_PIN); /* Set spark output pin low (active) */
//                     SerialTXStr(str_spark_01);
//                     sparks = true;
//                 }

//                 if ((sparks == true) & (spark_delay-- == 0)) {
//                     ShowError(ERR_IGNITION); /* ERROR: FLAME DIDN'T IGNITE */
//                 }

//             } else {
//                 SPARK_PORT |= (1 << SPARK_PIN);
//                 sparks = false;
//                 spark_delay = SPARK_DELAY;

//                 if (valve_1_delay-- == 0) {
//                     if (valve_1 == true) {
//                         VALVE_1_PORT &= ~(1 << VALVE_1_PIN);
//                         SerialTXStr(str_valve_1_02);
//                         valve_1 = false;
//                     }
//                     if (valve_2 == false) {
//                         VALVE_2_PORT |= (1 << VALVE_2_PIN);
//                         SerialTXStr(str_valve_2_01);
//                         SerialTXStr(str_dhw_duty_01);
//                         valve_2 = true;
//                         valve_1_delay = VALVE_1_DELAY;
//                     }
//                 }
//             }

//         } else {
//             if (dhw_req == true) {
//                 dhw_req = false;
//                 SerialTXStr(str_dhw_rq_02);
//                 AllOff();
//                 SerialTXStr(str_dhw_duty_02);
//                 valve_1_delay = VALVE_1_DELAY;
//             }
//         }
//     }

//     return 0;
// }

// /*   ___________________________
//     |                           |
//     |     All actuators off     |
//     |___________________________|
// */
// void AllOff(void) {
//     if (sparks == true) {
//         SerialTXStr(str_spark_02);
//         SPARK_PORT |= (1 << SPARK_PIN);
//         sparks = false;
//     }

//     SerialTXStr(str_gas_off);

//     VALVE_3_PORT &= ~(1 << VALVE_3_PIN);
//     valve_3 = false;

//     VALVE_2_PORT &= ~(1 << VALVE_2_PIN);
//     valve_2 = false;

//     VALVE_1_PORT &= ~(1 << VALVE_1_PIN);
//     valve_1 = false;

//     VALVE_S_PORT &= ~(1 << VALVE_S_PIN);
//     valve_s = false;

//     SerialTXStr(str_fan_02);

//     FAN_PORT &= ~(1 << FAN_PIN);
//     fan = false;
// }
