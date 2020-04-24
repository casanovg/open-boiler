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
#define DATA_LNG 10
    char str[DATA_LNG] = {0};
    switch (digits) {
        case DIGITS_1: {
            sprintf(str, "%1lu", (unsigned long int)data);
            break;
        }
        case DIGITS_2: {
            sprintf(str, "%2lu", (unsigned long int)data);
            break;
        }
        case DIGITS_3: {
            sprintf(str, "%3lu", (unsigned long int)data);
            break;
        }
        case DIGITS_4: {
            sprintf(str, "%4lu", (unsigned long int)data);
            break;
        }
        case DIGITS_5: {
            sprintf(str, "%5lu", (unsigned long int)data);
            break;
        }
        case DIGITS_6: {
            sprintf(str, "%6lu", (unsigned long int)data);
            break;
        }
        case DIGITS_7: {
            //sprintf(str, "%07lu", data);
            sprintf(str, "%7lu", (unsigned long int)data);
            break;
        }
        case DIGITS_8: {
            sprintf(str, "%8lu", (unsigned long int)data);
            break;
        }
        case DIGITS_9: {
            sprintf(str, "%9lu", (unsigned long int)data);
            break;
        }
        case DIGITS_10: {
            sprintf(str, "%10lu", (unsigned long int)data);
            break;
        }                        
        case DIGITS_FREE:
        default: {
            sprintf(str, "%lu", (unsigned long int)data);
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
        //SerialTxChr(61);
        SerialTxChr(line_char);
    }
}

// Function ClrScr: Clears the serial terminal screen
void ClrScr(void) {
    for (uint8_t i = 0; i < (sizeof(clr_ascii) / sizeof(clr_ascii[0])); i++) {
        SerialTxChr(clr_ascii[i]);
    }
}

// Function Dashboard
void Dashboard(SysInfo *p_sys, bool force_display) {

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
        if (GetFlag(p_sys, INPUT_FLAGS, OVERHEAT_F)) {
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
        if (GetFlag(p_sys, INPUT_FLAGS, DHW_REQUEST_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        //CH Request
        SerialTxStr(str_lit_01);
        if (GetFlag(p_sys, INPUT_FLAGS, CH_REQUEST_F)) {
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
        if (GetFlag(p_sys, INPUT_FLAGS, AIRFLOW_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Flame
        SerialTxStr(str_lit_03);
        if (GetFlag(p_sys, INPUT_FLAGS, FLAME_F)) {
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
        //SerialTxNum(p_sys->dhw_setting, DIGITS_4);
        //SerialTxChr(32); /* Space (_) */
        SerialTxNum(GetKnobPosition(p_sys->dhw_setting, DHW_SETTING_STEPS), DIGITS_2);
        SerialTxChr(32);
        SerialTxChr(32);

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        SerialTxStr(str_lit_16);
        //SerialTxNum(p_sys->ch_setting, DIGITS_4);
        //SerialTxChr(32); /* Space (_) */
        SerialTxNum(GetKnobPosition(p_sys->ch_setting, CH_SETTING_STEPS), DIGITS_2);
        SerialTxChr(32);
        SerialTxChr(32);

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        SerialTxStr(str_lit_17);
        //SerialTxNum(p_sys->system_mode, DIGITS_4);
        //SerialTxChr(32); /* Space (_) */
        
        switch (GetKnobPosition(p_sys->system_mode, SYSTEM_MODE_STEPS)) {
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
        if (GetFlag(p_sys, OUTPUT_FLAGS, EXHAUST_FAN_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Water pump
        SerialTxStr(str_lit_06);
        if (GetFlag(p_sys, OUTPUT_FLAGS, WATER_PUMP_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Spark igniter
        SerialTxStr(str_lit_07);
        if (GetFlag(p_sys, OUTPUT_FLAGS, SPARK_IGNITER_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // LED UI
        SerialTxStr(str_lit_12);
        if (GetFlag(p_sys, OUTPUT_FLAGS, LED_UI_F)) {
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
        if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_S_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Valve 1
        SerialTxStr(str_lit_09);
        if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_1_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Valve 2
        SerialTxStr(str_lit_10);
        if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_2_F)) {
            SerialTxStr(str_true);
        } else {
            SerialTxStr(str_false);
        }

        SerialTxChr(32); /* Space (_) */
        SerialTxChr(32); /* Space (_) */

        // Valve 3
        SerialTxStr(str_lit_11);
        if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_3_F)) {
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
        //SerialTxNum(p_sys->pump_delay, DIGITS_7);
        //SerialTxNum((unsigned long)GetTimeLeft(PUMP_TIMER_ID), DIGITS_7);
        //SerialTxNum(GetTimeLeft, DIGITS_7);
        SerialTxStr(str_crlf);
#endif /* SHOW_PUMP_TIMER */
    }

}
