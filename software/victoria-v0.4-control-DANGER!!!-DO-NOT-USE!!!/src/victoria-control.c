/*
 *  Open-Boiler Control
 *  ....................................
 *  File: victoria-control.c
 *  ....................................
 *  v0.4 / 2019-08-08 / ATmega328
 *  ....................................
 *  Gustavo Casanova / Nicebots
 */

#include "victoria-control.h"

#define DLY_L1 0xFFFF
#define DLY_L2 0x0C

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
    //State system_state = OFF;
    SysInfo sys_info;
    SysInfo *p_system = &sys_info;
    p_system->system_state = OFF;

    IgnitionState ignition_state = IG_1;

    uint16_t dhw_sensor = 0;
    uint16_t ceh_sensor = 0;
    uint16_t dhw_pot = 0;
    uint16_t ceh_pot = 0;
    uint16_t sys_pot = 0;

    static int delay_l1 = DLY_L1;
    static uint8_t delay_l2 = DLY_L2;

    InitFlags(p_system, INPUT_FLAGS);

    for (OutputFlag device = EXHAUST_FAN; device <= LED_UI; device++) {
        InitActuator(p_system, device);
    }

    for (InputFlag sensor = DHW_REQUEST; sensor <= OVERHEAT; sensor++) {
        InitSensor(p_system, sensor);
    }

    SerialInit();
    _delay_ms(1000);

    p_system->pre_iflags = 0;
    p_system->pre_oflags = 0;

    //Dashboard(p_system);

    /*  ___________________
       |                   | 
       |     Main Loop     |
       |___________________|
    */
    for (;;) {
        for (InputFlag sensor = DHW_REQUEST; sensor <= OVERHEAT; sensor++) {
            CheckSensor(p_system, sensor);
        }
        // Display only if something changed
        // if((pre_if != p_system->input_flags) || (pre_of != p_system->output_flags)) {
        //     ClrScr();
        //     Dashboard(p_system);
        //     pre_if = p_system->input_flags;
        //     pre_of = p_system->output_flags;
        // }
        switch (p_system->system_state) {
            // .......................
            // . System state -> OFF  .
            // .......................
            case OFF: {
                Dashboard(p_system);
                if (!((p_system->output_flags >> EXHAUST_FAN) & true)) {
                    // At this point, with the exhaust fan turned off, all sensors should be off, otherwise, something is failing
                    //if (((p_system->pre_iflags >> FLAME_SENSOR) & true) | ((p_system->pre_iflags >> OVERHEAT) & true) | ((p_system->pre_iflags >> AIRFLOW) & true)) {
                    if (((p_system->pre_iflags >> FLAME_SENSOR) & true) | ((p_system->pre_iflags >> OVERHEAT) & true)) {
                        delay_l1 = DLY_L1;
                        delay_l2 = DLY_L2;
                        p_system->system_state = ERROR;
                    } else {
                        ControlActuator(p_system, EXHAUST_FAN, TURN_ON);
                    }
                } else {
                    // With the exhaust fan turned on, we wait until the airflow sensor closes, or exit on timeout
                    // Other sensors should be off
                    if (((p_system->pre_iflags >> FLAME_SENSOR) & true) | ((p_system->pre_iflags >> OVERHEAT) & true)) {
                        delay_l1 = DLY_L1;
                        delay_l2 = DLY_L2;
                        ControlActuator(p_system, EXHAUST_FAN, TURN_OFF);
                        p_system->system_state = ERROR;
                    }
                    if (!delay_l1--) {
                        delay_l1 = DLY_L1;
                        if (!delay_l2--) {
                            delay_l2 = DLY_L2;
                            ControlActuator(p_system, EXHAUST_FAN, TURN_OFF);
                            p_system->system_state = ERROR;
                        }
                    }
                    if ((p_system->pre_iflags >> AIRFLOW) & true) {
                        delay_l1 = DLY_L1;
                        delay_l2 = DLY_L2;
                        ControlActuator(p_system, EXHAUST_FAN, TURN_OFF);
                        p_system->system_state = READY;
                    }
                }
                break;
            }
            // .........................
            // . System state -> READY  .
            // .........................
            case READY: {
                Dashboard(p_system);
                if (!delay_l1--) {
                    delay_l1 = DLY_L1;
                    if (!delay_l2--) {
                        delay_l2 = DLY_L2;
                        if (((p_system->pre_iflags >> DHW_REQUEST) & true) | ((p_system->pre_iflags >> CEH_REQUEST) & true)) {
                            delay_l1 = DLY_L1;
                            delay_l2 = DLY_L2;
                            p_system->system_state = IGNITING;
                        }
                        //if (((p_system->pre_iflags >> FLAME_SENSOR) & true) | ((p_system->pre_iflags >> AIRFLOW) & true) | ((p_system->pre_iflags >> OVERHEAT) & true)) {
                            if (((p_system->pre_iflags >> FLAME_SENSOR) & true) | ((p_system->pre_iflags >> OVERHEAT) & true)) {
                            p_system->system_state = ERROR;
                        }
                    }
                }
                break;
            }
            // ............................
            // . System state -> IGNITING  .
            // ............................
            case IGNITING: {
                Dashboard(p_system);
                // If the request is canceled, return to "ready" state
                if ((~((p_system->pre_iflags >> DHW_REQUEST) & true)) & (~((p_system->pre_iflags >> CEH_REQUEST) & true))) {
                    // Closing gas ...
                    _delay_ms(50); /* REVIEW THIS DELAY */
                    ControlActuator(p_system, VALVE_1, TURN_OFF);
                    _delay_ms(50); /* REVIEW THIS DELAY */
                    ControlActuator(p_system, VALVE_S, TURN_OFF);
                    if ((p_system->pre_oflags >> SPARK_IGNITER) & true) {
                        ControlActuator(p_system, SPARK_IGNITER, TURN_OFF);
                    }
                    p_system->system_state = READY;
                }
                switch (ignition_state) {
                    // WARNING! Opening gas valve 1!
                    case IG_1: {
                        _delay_ms(100); /* REVIEW THIS DELAY */
                        ControlActuator(p_system, VALVE_S, TURN_ON);
                        _delay_ms(100); /* REVIEW THIS DELAY */
                        ControlActuator(p_system, VALVE_1, TURN_ON);
                        _delay_ms(250); /* REVIEW THIS DELAY */
                        ignition_state = IG_2;
                        break;
                    }
                    case IG_2: {
                        ControlActuator(p_system, SPARK_IGNITER, TURN_ON);
                        ignition_state = IG_3;
                        break;
                    }
                    case IG_3: {
                        if ((p_system->output_flags >> FLAME_SENSOR) & true) {
                            ignition_state = IG_OK;
                        } else {
                            if (!delay_l1--) {
                                delay_l1 = DLY_L1;
                                if (!delay_l2--) {
                                    delay_l2 = DLY_L2;
                                    ignition_state = IG_4;
                                }
                            }
                        }
                        break;
                    }
                    case IG_4: {
                        // WARNING! Opening gas valve 2!
                        _delay_ms(20);
                        ControlActuator(p_system, VALVE_1, TURN_OFF);
                        _delay_ms(100);
                        ControlActuator(p_system, VALVE_2, TURN_ON);
                        _delay_ms(50);
                        delay_l1 = DLY_L1;
                        delay_l2 = DLY_L2;
                        ignition_state = IG_5;
                        break;
                    }
                    case IG_5: {
                        if ((p_system->output_flags >> FLAME_SENSOR) & true) {
                            ignition_state = IG_OK;
                        } else {
                            if (!delay_l1--) {
                                delay_l1 = DLY_L1;
                                if (!delay_l2--) {
                                    delay_l2 = DLY_L2;
                                    ignition_state = IG_ERR;
                                }
                            }
                        }
                        break;
                    }
                    case IG_OK: {
                        delay_l1 = DLY_L1;
                        delay_l2 = DLY_L2;
                        if ((p_system->output_flags >> DHW_REQUEST) & true) {
                            p_system->system_state = ON_DHW_DUTY;
                        } else if ((p_system->output_flags >> CEH_REQUEST) & true) {
                            p_system->system_state = ON_CEH_DUTY;
                        } else {
                            // Closing gas ...
                            delay_l1 = DLY_L1;
                            delay_l2 = DLY_L2;
                            _delay_ms(50); /* REVIEW THIS DELAY */
                            ControlActuator(p_system, VALVE_1, TURN_OFF);
                            _delay_ms(50); /* REVIEW THIS DELAY */
                            ControlActuator(p_system, VALVE_S, TURN_OFF);
                            if ((p_system->pre_oflags >> SPARK_IGNITER) & true) {
                                ControlActuator(p_system, SPARK_IGNITER, TURN_OFF);
                            }
                            p_system->system_state = READY;                            
                        }
                        break;
                    }
                    case IG_ERR: {
                        delay_l1 = DLY_L1;
                        delay_l2 = DLY_L2;
                        _delay_ms(50); /* REVIEW THIS DELAY */
                        ControlActuator(p_system, VALVE_1, TURN_OFF);
                        _delay_ms(50); /* REVIEW THIS DELAY */
                        ControlActuator(p_system, VALVE_S, TURN_OFF);
                        if ((p_system->pre_oflags >> SPARK_IGNITER) & true) {
                            ControlActuator(p_system, SPARK_IGNITER, TURN_OFF);
                        }                        
                        p_system->system_state = ERROR;
                        break;
                    }
                }
                break;
            }
            // ...............................
            // . System state -> ON_DHW_DUTY  .
            // ...............................
            case ON_DHW_DUTY: {
                Dashboard(p_system);
                if ((~((p_system->pre_iflags >> DHW_REQUEST) & true)) & (~((p_system->pre_iflags >> CEH_REQUEST) & true))) {
                    // Closing gas ...
                    _delay_ms(50); /* REVIEW THIS DELAY */
                    ControlActuator(p_system, VALVE_1, TURN_OFF);
                    _delay_ms(50); /* REVIEW THIS DELAY */
                    ControlActuator(p_system, VALVE_S, TURN_OFF);
                    if ((p_system->pre_oflags >> SPARK_IGNITER) & true) {
                        ControlActuator(p_system, SPARK_IGNITER, TURN_OFF);
                    }
                    p_system->system_state = READY;
                }
                //if ( (~((p_system->pre_iflags >> FLAME_SENSOR) & true)) | (~((p_system->pre_iflags >> AIRFLOW) & true)) | (~((p_system->pre_iflags >> OVERHEAT) & true)) ) {
                if (~((p_system->pre_iflags >> FLAME_SENSOR) & true)) {
                    // Closing gas ...
                    _delay_ms(50); /* REVIEW THIS DELAY */
                    ControlActuator(p_system, VALVE_1, TURN_OFF);
                    _delay_ms(50); /* REVIEW THIS DELAY */
                    ControlActuator(p_system, VALVE_S, TURN_OFF);
                    if ((p_system->pre_oflags >> SPARK_IGNITER) & true) {
                        ControlActuator(p_system, SPARK_IGNITER, TURN_OFF);
                    }                    
                    p_system->system_state = ERROR;
                }                
                
                break;
            }
            // ...............................
            // . System state -> ON_CEH_DUTY  .
            // ...............................
            case ON_CEH_DUTY: {
                // Closing gas ...
                _delay_ms(50); /* REVIEW THIS DELAY */
                ControlActuator(p_system, VALVE_1, TURN_OFF);
                _delay_ms(50); /* REVIEW THIS DELAY */
                ControlActuator(p_system, VALVE_S, TURN_OFF);
                if ((p_system->pre_oflags >> SPARK_IGNITER) & true) {
                    ControlActuator(p_system, SPARK_IGNITER, TURN_OFF);
                }             
                p_system->system_state = READY;
                break;
            }
            // .........................
            // . System state -> ERROR  .
            // .........................
            case ERROR: {
                Dashboard(p_system);
                for (OutputFlag device = EXHAUST_FAN; device <= LED_UI; device++) {
                    ControlActuator(p_system, device, TURN_OFF);
                }
                uint8_t blinks = 200;
                while (blinks-- != 0) {
                    ControlActuator(p_system, LED_UI, TURN_ON);
                    _delay_ms(125);
                    ControlActuator(p_system, LED_UI, TURN_OFF);
                    _delay_ms(125);
                }
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
void SetFlag(SysInfo *p_sys, FlagsByte flags_byte, uint8_t flag_position) {
    switch (flags_byte) {
        case INPUT_FLAGS: {
            p_sys->input_flags |= (1 << flag_position);
            break;
        }
        case OUTPUT_FLAGS: {
            p_sys->output_flags |= (1 << flag_position);
            //ControlActuator(flag_position, TURN_ON);
            break;
        }
        default: {
            break;
        }
    }
}

// Function ClearFlag
void ClearFlag(SysInfo *p_sys, FlagsByte flags_byte, uint8_t flag_position) {
    switch (flags_byte) {
        case INPUT_FLAGS: {
            p_sys->input_flags &= ~(1 << flag_position);
            break;
        }
        case OUTPUT_FLAGS: {
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
        }
        case OUTPUT_FLAGS: {
            return ((p_sys->output_flags >> flag_position) & true);
        }
        default: {
            break;
        }
    }
}

// Function InitSensor
void InitSensor(SysInfo *p_sys, InputFlag sensor) {
    switch (sensor) {
        case DHW_REQUEST: {
            DHW_RQ_DDR &= ~(1 << DHW_RQ_PIN); /* Set DHW request pin as input */
            DHW_RQ_PORT |= (1 << DHW_RQ_PIN); /* Activate pull-up resistor on this pin */
            break;
        }
        case CEH_REQUEST: {
            CEH_RQ_DDR &= ~(1 << CEH_RQ_PIN); /* Set CEH request pin as input */
            CEH_RQ_PORT |= (1 << CEH_RQ_PIN); /* Activate pull-up resistor on this pin */
            break;
        }
        case AIRFLOW: {
            AIRFLOW_DDR &= ~(1 << AIRFLOW_PIN); /* Set Airflow detector pin as input */
            AIRFLOW_PORT |= (1 << AIRFLOW_PIN); /* Activate pull-up resistor on this pin */
            break;
        }
        case FLAME_SENSOR: {
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
    ClearFlag(p_sys, INPUT_FLAGS, sensor);
}

// Function CheckSensor
bool CheckSensor(SysInfo *p_sys, InputFlag sensor) {
    switch (sensor) {
        case DHW_REQUEST: {
            if ((DHW_RQ_PINP >> DHW_RQ_PIN) & true) {
                p_sys->input_flags &= ~(1 << DHW_REQUEST);
            } else {
                p_sys->input_flags |= (1 << DHW_REQUEST);
            }
            return ((p_sys->input_flags >> DHW_REQUEST) & true);
            break;
        }
        case CEH_REQUEST: {
            if ((CEH_RQ_PINP >> CEH_RQ_PIN) & true) {
                p_sys->input_flags &= ~(1 << CEH_REQUEST);
            } else {
                p_sys->input_flags |= (1 << CEH_REQUEST);
            }
            return ((p_sys->input_flags >> CEH_REQUEST) & true);
            break;
        }
        case AIRFLOW: {
            if ((AIRFLOW_PINP >> AIRFLOW_PIN) & true) {
                p_sys->input_flags &= ~(1 << AIRFLOW);
            } else {
                p_sys->input_flags |= (1 << AIRFLOW);
            }
            return ((p_sys->input_flags >> AIRFLOW) & true);
            break;
        }
        case FLAME_SENSOR: { /* IT NEEDS EXTERNAL PULL-DOWN RESISTOR !!! */
            if ((FLAME_PINP >> FLAME_PIN) & true) {
                p_sys->input_flags |= (1 << FLAME_SENSOR);
            } else {
                p_sys->input_flags &= ~(1 << FLAME_SENSOR);
            }
            return ((p_sys->input_flags >> FLAME_SENSOR) & true);
            break;
        }
        case OVERHEAT: { /* OVERHEAD CHECKING DISABLED !!! UNCOMMENT FOR PROD */
            // if ((OVERHEAT_PINP >> OVERHEAT_PIN) & true) {
            //     p_sys->input_flags &= ~(1 << OVERHEAT);
            // } else {
            //     p_sys->input_flags |= (1 << OVERHEAT);
            // }
            p_sys->input_flags &= ~(1 << OVERHEAT); /* FORCED TO NOT-OVERHEAT !!! COMMENT FOR PROD */
            return ((p_sys->input_flags >> OVERHEAT) & true);
            break;
        }
    }
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
    ClearFlag(p_sys, OUTPUT_FLAGS, device);
}

// Function ControlActuator
void ControlActuator(SysInfo *p_sys, OutputFlag device, HwSwitch command) {
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
                VALVE_2_PORT &= ~(1 << VALVE_1_PIN); /* Set valve 2 pin low (inactive) */
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
    if (command == TURN_ON) {
        SetFlag(p_sys, OUTPUT_FLAGS, device);
    } else {
        ClearFlag(p_sys, OUTPUT_FLAGS, device);
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
void SerialTxNum(uint16_t data) {
#define DATA_LNG 5
    char str[DATA_LNG] = {0};
    //uint8_t non_zero = 0;
    sprintf(str, "%02d", data);
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
void Dashboard(SysInfo *p_sys) {
#define DASH_WIDTH 82

    if ((p_sys->input_flags != p_sys->pre_iflags) | (p_sys->output_flags != p_sys->pre_oflags)) {
        p_sys->pre_iflags = p_sys->input_flags;
        p_sys->pre_oflags = p_sys->output_flags;

        ClrScr();

        DrawLine(DASH_WIDTH, 61); /* Dashed line */
        SerialTxStr(str_crlf);    /* CR + new line */

        SerialTxStr(str_header_01);
        SerialTxStr(str_header_02);
        //SerialTxStr("[[[ ");
        //SerialTxNum(p_sys->system_state);
        //SerialTxStr(" ]]]\n\r");

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
            case ON_CEH_DUTY: {
                SerialTxStr(str_mode_40);
                break;
            }
            case ERROR: {
                SerialTxStr(str_mode_100);
                break;
            }
        }

        SerialTxChr(124);             /* Horizontal separator (|) */
        SerialTxChr(32);              /* Space (_) */
        DrawLine(DASH_WIDTH - 3, 46); /* Dotted line */
        SerialTxStr(str_crlf);        /* CR + new line */

        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        // Input flags
        SerialTxStr(str_iflags);
        SerialTxNum(p_sys->input_flags);
        SerialTxChr(32); /* Space (_) */

        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        // DHW temperature
        SerialTxStr(str_lit_13);
        SerialTxNum(0);

        SerialTxChr(32);  /* Space (_) */
        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        // CH temperature
        SerialTxStr(str_lit_14);
        SerialTxNum(0);

        SerialTxStr(str_crlf); /* CR + new line */

        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        // DHW Request
        SerialTxStr(str_lit_00);
        if (GetFlag(p_sys, INPUT_FLAGS, DHW_REQUEST)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32);  /* Space (_) */
        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        //CH Request
        SerialTxStr(str_lit_01);
        if (GetFlag(p_sys, INPUT_FLAGS, CEH_REQUEST)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32);  /* Space (_) */
        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        // Airflow
        SerialTxStr(str_lit_02);
        if (GetFlag(p_sys, INPUT_FLAGS, AIRFLOW)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32);  /* Space (_) */
        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        // Flame
        SerialTxStr(str_lit_03);
        if (GetFlag(p_sys, INPUT_FLAGS, FLAME_SENSOR)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32);  /* Space (_) */
        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        // Overheat
        SerialTxStr(str_lit_04);
        if (GetFlag(p_sys, INPUT_FLAGS, OVERHEAT)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxStr(str_crlf);        /* CR + new line */
        SerialTxChr(124);             /* Horizontal separator (|) */
        SerialTxChr(32);              /* Space (_) */
        DrawLine(DASH_WIDTH - 3, 46); /* Dotted line */
        SerialTxStr(str_crlf);        /* CR + new line */
        SerialTxChr(124);             /* Horizontal separator (|) */
        SerialTxChr(32);              /* Space (_) */

        SerialTxStr(str_oflags);
        SerialTxNum(p_sys->output_flags);
        SerialTxStr(str_crlf); /* CR + new line */

        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        // Exhaust fan
        SerialTxStr(str_lit_05);
        if (GetFlag(p_sys, OUTPUT_FLAGS, EXHAUST_FAN)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32);  /* Space (_) */
        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        // Water pump
        SerialTxStr(str_lit_06);
        if (GetFlag(p_sys, OUTPUT_FLAGS, WATER_PUMP)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32);  /* Space (_) */
        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        // Spark igniter
        SerialTxStr(str_lit_07);
        if (GetFlag(p_sys, OUTPUT_FLAGS, SPARK_IGNITER)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32);  /* Space (_) */
        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        // LED UI
        SerialTxStr(str_lit_12);
        if (GetFlag(p_sys, OUTPUT_FLAGS, LED_UI)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxStr(str_crlf); /* CR + new line */
        SerialTxChr(124);      /* Horizontal separator (|) */
        SerialTxChr(32);       /* Space (_) */

        // Security valve
        SerialTxStr(str_lit_08);
        if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_S)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        // Valve 1
        SerialTxStr(str_lit_09);
        if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_1)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        // Valve 2
        SerialTxStr(str_lit_10);
        if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_2)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(124); /* Horizontal separator (|) */
        SerialTxChr(32);  /* Space (_) */

        // Valve 3
        SerialTxStr(str_lit_11);
        if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_3)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxStr(str_crlf);    /* CR + new line */
        DrawLine(DASH_WIDTH, 61); /* Dashed line */
        SerialTxStr(str_crlf);    /* CR + new line */
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
