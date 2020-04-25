/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: serial-ui.c (serial user interface library)
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-09 (Easter Quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#include "serial-ui.h"

// Function SerialInit
void SerialInit(void) {
    UBRR0H = (uint8_t)(BAUD_PRESCALER >> 8);
    UBRR0L = (uint8_t)(BAUD_PRESCALER);
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (3 << UCSZ00);
}

// Function SerialRxChr
uint8_t SerialRxChr(void) {
    while (!(UCSR0A & (1 << RXC0))) {
    };
    return UDR0;
}

// Function SerialTxChr
void SerialTxChr(uint8_t character_code) {
    while (!(UCSR0A & (1 << UDRE0))) {
    };
    UDR0 = character_code;
}

// Function SerialTxNum
void SerialTxTemp(int ntc_temperature) {
#define STR_NUM 5
#define DOT 46
    uint8_t str_num[STR_NUM] = {0};
    sprintf(str_num, "%d", ((int)ntc_temperature));
    uint8_t str_len = strlen(str_num);
    // Integer part
    for (int i = 0; i < str_len - 1; i++) {
        while (!(UCSR0A & (1 << UDRE0))) {
        };
        UDR0 = str_num[i];
    }
    // Decimal separator
    while (!(UCSR0A & (1 << UDRE0))) {
    };
    UDR0 = DOT;
    // Decimal part
    while (!(UCSR0A & (1 << UDRE0))) {
    };
    UDR0 = str_num[strlen(str_num) - 1];
}

// Function SerialTxNum
void SerialTxNum(uint32_t number, DigitLength digits) {
#define DATA_LNG 7
    char str[DATA_LNG] = {0};
    switch (digits) {
        case DIGITS_1: {
            sprintf(str, "%01u", (uint16_t)number);
            break;
        }
        case DIGITS_2: {
            sprintf(str, "%02u", (uint16_t)number);
            break;
        }
        case DIGITS_3: {
            sprintf(str, "%03u", (uint16_t)number);
            break;
        }
        case DIGITS_4: {
            sprintf(str, "%04u", (uint16_t)number);
            break;
        }
        case DIGITS_5: {
            sprintf(str, "%05u", (uint16_t)number);
            break;
        }
        case DIGITS_6: {
            sprintf(str, "%06u", (uint16_t)number);
            break;
        }
        case DIGITS_7:
        case DIGITS_8:
        case DIGITS_9:
        case DIGITS_10: {
            sprintf(str, "%0lu", (uint32_t)number);
            break;
        }
        case TEMP_NN: {
            sprintf(str, "%02u", DivRound((uint16_t)number, 10));
            break;
        }
        case TEMP_DD: {
            //sprintf(str, "%1u", DivRound(((uint16_t)data % 10), 10));
            sprintf(str, "%1u", (uint16_t)number % 10);
            break;
        }

        case ZUZU: {
            sprintf(str, "%d", ((int)number));
        }

        case DIGITS_FREE:
        default: {
            sprintf(str, "%u", (uint16_t)number);
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

// Function DrawDashedLine
void DrawLine(uint8_t length, char line_char) {
    for (uint8_t i = 0; i < length; i++) {
        SerialTxChr(line_char);
    }
}

// Function DivRound
int DivRound(const int numerator, const int denominator) {
    int result = 0;
    if ((numerator < 0) ^ (denominator < 0)) {
        result = ((numerator - denominator / 2) / denominator);
    } else {
        result = ((numerator + denominator / 2) / denominator);
    }
    return result;
}

// Function ClrScr: Clears the serial terminal screen
void ClrScr(void) {
    for (uint8_t i = 0; i < (sizeof(clr_ascii) / sizeof(clr_ascii[0])); i++) {
        SerialTxChr(clr_ascii[i]);
    }
}

#if SHOW_DASHBOARD

// Function Dashboard
void Dashboard(SysInfo *p_system, bool force_refresh) {
    if (force_refresh |
        (p_system->input_flags != p_system->last_displayed_iflags) |
        (p_system->output_flags != p_system->last_displayed_oflags)) {
        p_system->last_displayed_iflags = p_system->input_flags;
        p_system->last_displayed_oflags = p_system->output_flags;

        ClrScr();

        DrawLine(DASH_WIDTH, H_ELINE);  // Dashed line (= 61)

        SerialTxStr(str_crlf);  // CR + new line

        SerialTxChr(V_LINE);  // Horizontal separator (|)
        SerialTxStr(str_header_01);
        SerialTxStr(str_header_02);

        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)

        // Mode display
        switch (p_system->system_state) {
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
                switch (p_system->inner_step) {
                    case CH_ON_DUTY_1: {
                        SerialTxStr(str_mode_41);
                        break;
                    }
                    case CH_ON_DUTY_2: {
                        SerialTxStr(str_mode_42);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case ERROR: {
                SerialTxStr(str_mode_100);
                break;
            }
        }

        SerialTxChr(V_LINE);  // Horizontal separator (|)
        SerialTxChr(SPACE);   // Space (_)

        DrawLine(DASH_WIDTH - 4, H_ILINE);  // Dotted line
        SerialTxChr(SPACE);
        SerialTxChr(V_LINE);  // Horizontal separator (|)

        SerialTxStr(str_crlf);  // CR + new line

        SerialTxChr(V_LINE);  // Horizontal separator (|)
        SerialTxChr(SPACE);   // Space (_)

        // Input flags
        SerialTxStr(str_iflags);
        SerialTxNum(p_system->input_flags, DIGITS_2);

        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)

        // DHW temperature
        int dhw_temperature = GetNtcTemperature(p_system->dhw_temperature, TO_CELSIUS, DT_CELSIUS);
        SerialTxStr(str_lit_13);
        if (dhw_temperature != -32767) {
            SerialTxNum(dhw_temperature, TEMP_NN);
            SerialTxChr(V_LINE);
            SerialTxNum(dhw_temperature, TEMP_DD);
        } else {
            SerialTxStr(str_temperr);
        }
        SerialTxChr(126);  // Tilde (~)
        SerialTxChr(67);   // C

        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)

        // CH temperature
        int ch_temperature = GetNtcTemperature(p_system->ch_temperature, TO_CELSIUS, DT_CELSIUS);
        SerialTxStr(str_lit_14);
        if (ch_temperature != -32767) {
            SerialTxNum(ch_temperature, TEMP_NN);
            SerialTxChr(V_LINE);
            SerialTxNum(ch_temperature, TEMP_DD);
        } else {
            SerialTxStr(str_temperr);
        }
        SerialTxChr(126);    // Tilde (~)
        SerialTxChr(67);     // C
        SerialTxChr(SPACE);  // Space (_)
        SerialTxNum(p_system->ch_temperature, DIGITS_4);
        SerialTxChr(SPACE);  // Space (_)

        // Overheat
#if !(OVERHEAT_OVERRIDE)
        SerialTxStr(str_lit_04);
#else
        SerialTxStr(str_lit_04_override);
#endif  // OVERHEAT_OVERRIDE
        if (GetFlag(p_system, INPUT_FLAGS, OVERHEAT_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(SPACE);   // Space (_)
        SerialTxChr(V_LINE);  // Horizontal separator (|)

        SerialTxStr(str_crlf);  // CR + new line

        SerialTxChr(V_LINE);  // Horizontal separator (|)
        SerialTxChr(SPACE);   // Space (_)

        // DHW Request
        SerialTxStr(str_lit_00);
        if (GetFlag(p_system, INPUT_FLAGS, DHW_REQUEST_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)

        //CH Request
        SerialTxStr(str_lit_01);
        if (GetFlag(p_system, INPUT_FLAGS, CH_REQUEST_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)

        // Airflow
#if !(AIRFLOW_OVERRIDE)
        SerialTxStr(str_lit_02);
#else
        SerialTxStr(str_lit_02_override);
#endif  // AIRFLOW_OVERRIDE
        if (GetFlag(p_system, INPUT_FLAGS, AIRFLOW_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)

        // Flame
        SerialTxStr(str_lit_03);
        if (GetFlag(p_system, INPUT_FLAGS, FLAME_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(SPACE);   // Space (_)
        SerialTxChr(SPACE);   // Space (_)
        SerialTxChr(V_LINE);  // Horizontal separator (|)

        SerialTxStr(str_crlf);  // CR + new line

        SerialTxChr(V_LINE);  // Horizontal separator (|)
        SerialTxChr(SPACE);   // Space (_)

        DrawLine(DASH_WIDTH - 4, H_ILINE);  // Dotted line
        SerialTxChr(SPACE);                 // Space (_)
        SerialTxChr(V_LINE);                // Horizontal separator (|)

        SerialTxStr(str_crlf);  // CR + new line

        SerialTxChr(V_LINE);  // Horizontal separator (|)
        SerialTxChr(SPACE);   // Space (_)

        SerialTxStr(str_lit_18);
        SerialTxChr(SPACE);  // Space (_)

        SerialTxStr(str_lit_15);
        SerialTxNum(GetKnobPosition(p_system->dhw_setting, DHW_SETTING_STEPS), DIGITS_2);
        SerialTxChr(SPACE);
        SerialTxChr(SPACE);

        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)

        SerialTxStr(str_lit_16);
        SerialTxNum(GetKnobPosition(p_system->ch_setting, CH_SETTING_STEPS), DIGITS_2);
        SerialTxChr(SPACE);
        SerialTxChr(SPACE);

        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)

        SerialTxStr(str_lit_17);
        switch (GetKnobPosition(p_system->system_mode, SYSTEM_MODE_STEPS)) {
            case SYS_COMBI: {
                SerialTxStr(sys_mode_00);
                break;
            }
            case SYS_DHW: {
                SerialTxStr(sys_mode_01);
                break;
            }
            case SYS_OFF: {
                SerialTxStr(sys_mode_02);
                break;
            }
            case SYS_RESET: {
                SerialTxStr(sys_mode_03);
                break;
            }
        }

        SerialTxChr(V_LINE);  // Horizontal separator (|)

        SerialTxStr(str_crlf);  // CR + new line

        SerialTxChr(V_LINE);  // Horizontal separator (|)
        SerialTxChr(SPACE);   // Space (_)

        DrawLine(DASH_WIDTH - 4, H_ILINE);  // Dotted line
        SerialTxChr(SPACE);
        SerialTxChr(V_LINE);  // Horizontal separator (|)

        SerialTxStr(str_crlf);

        SerialTxChr(V_LINE);  // Horizontal separator (|)
        SerialTxChr(SPACE);   // Space (_)

        SerialTxStr(str_oflags);
        SerialTxNum(p_system->output_flags, DIGITS_2);

        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)

        // Exhaust fan
        SerialTxStr(str_lit_05);
        if (GetFlag(p_system, OUTPUT_FLAGS, EXHAUST_FAN_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)

        // Water pump
        SerialTxStr(str_lit_06);
        if (GetFlag(p_system, OUTPUT_FLAGS, WATER_PUMP_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        if (p_system->ch_water_overheat) {
            SerialTxChr(SPACE);  // Space (_)
            SerialTxChr(42);     // Asterisk (*)
            SerialTxChr(SPACE);  // Space (_)
        } else {
            SerialTxChr(SPACE);  // Space (_)
            SerialTxChr(SPACE);  // Space (_)
            SerialTxChr(SPACE);  // Space (_)
        }

        SerialTxChr(SPACE);  // Space (_)

        // Spark igniter
        SerialTxStr(str_lit_07);
        if (GetFlag(p_system, OUTPUT_FLAGS, SPARK_IGNITER_F)) {
            SerialTxStr(str_true);
            SerialTxChr(SPACE);                               // Space (_)
            SerialTxChr(84);                                  // Displays ignition retries
            SerialTxNum(p_system->ignition_tries, DIGITS_1);  // Displays ignition retries
            //SerialTxChr(SPACE);                                 // Space (_)
        } else {
            SerialTxStr(str_false);
            SerialTxChr(SPACE);  // Space (_)
            SerialTxChr(SPACE);  // Space (_)
            SerialTxChr(SPACE);  // Space (_)
        }

        //SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)

        // LED UI
        SerialTxStr(str_lit_12);
        if (GetFlag(p_system, OUTPUT_FLAGS, LED_UI_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(SPACE);   // Space (_)
        SerialTxChr(SPACE);   // Space (_)
        SerialTxChr(V_LINE);  // Horizontal separator (|)

        SerialTxStr(str_crlf);  // CR + new line

        SerialTxChr(V_LINE);  // Horizontal separator (|)
        SerialTxChr(SPACE);   // Space (_)

        // Security valve
        SerialTxStr(str_lit_08);
        if (GetFlag(p_system, OUTPUT_FLAGS, VALVE_S_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)

        // Valve 1
        SerialTxStr(str_lit_09);
        if (GetFlag(p_system, OUTPUT_FLAGS, VALVE_1_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)

        // Valve 2
        SerialTxStr(str_lit_10);
        if (GetFlag(p_system, OUTPUT_FLAGS, VALVE_2_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(SPACE);  // Space (_)
        SerialTxChr(SPACE);  // Space (_)

        // Valve 3
        SerialTxStr(str_lit_11);
        if (GetFlag(p_system, OUTPUT_FLAGS, VALVE_3_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(SPACE);   // Space (_)
        SerialTxChr(V_LINE);  // Horizontal separator (|)

        SerialTxStr(str_crlf);          // CR + new line
        DrawLine(DASH_WIDTH, H_ELINE);  // Dashed line
        SerialTxStr(str_crlf);          // CR + new line

#if SHOW_PUMP_TIMER
        SerialTxStr(str_crlf);
        SerialTxStr(str_wptimer);
        //SerialTxNum(p_system->pump_delay, DIGITS_7);
        SerialTxNum(GetTimeLeft(PUMP_TIMER_ID) / 1000, DIGITS_7);
        //SerialTxNum(GetTimeLeft, DIGITS_7);

        if (p_system->pump_timer_memory) {
            SerialTxChr(SPACE);  // Space (_)
            SerialTxStr(str_wpmemory);
            SerialTxNum(p_system->pump_timer_memory / 1000, DIGITS_7);
            SerialTxStr(str_crlf);
        }
#endif  // SHOW_PUMP_TIMER
        SerialTxStr(str_crlf);
        SerialTxStr(str_crlf);
    }
}

#endif  // SHOW_DASHBOARD
