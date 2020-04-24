/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: serial-ui.c (serial user interface library)
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-09 (Easter quarantine)
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
    // #define DASH_WIDTH 65
    // #define H_ELINE 46
    // #define H_ILINE 46
    // #define V_LINE 46

    //     if (force_display |
    //         (p_sys->input_flags != p_sys->last_displayed_iflags) |
    //         (p_sys->output_flags != p_sys->last_displayed_oflags)) {
    //         p_sys->last_displayed_iflags = p_sys->input_flags;
    //         p_sys->last_displayed_oflags = p_sys->output_flags;

    //         ClrScr();

    //         DrawLine(DASH_WIDTH, H_ELINE); /* Dashed line (= 61) */
    //         SerialTxStr(str_crlf);         /* CR + new line */

    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */
    //         SerialTxStr(str_header_01);
    //         SerialTxStr(str_header_02);

    //         // Mode display
    //         switch (p_sys->system_state) {
    //             case OFF: {
    //                 SerialTxStr(str_mode_00);
    //                 break;
    //             }
    //             case READY: {
    //                 SerialTxStr(str_mode_10);
    //                 break;
    //             }
    //             case IGNITING: {
    //                 SerialTxStr(str_mode_20);
    //                 break;
    //             }
    //             case DHW_ON_DUTY: {
    //                 SerialTxStr(str_mode_30);
    //                 break;
    //             }
    //             case CH_ON_DUTY: {
    //                 SerialTxStr(str_mode_40);
    //                 break;
    //             }
    //             case ERROR: {
    //                 SerialTxStr(str_mode_100);
    //                 break;
    //             }
    //         }

    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */
    //         SerialTxChr(32);     /* Space (_) */

    //         DrawLine(DASH_WIDTH - 4, H_ILINE); /* Dotted line */
    //         SerialTxChr(32);
    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */

    //         SerialTxStr(str_crlf); /* CR + new line */

    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */
    //         SerialTxChr(32);     /* Space (_) */

    //         // Input flags
    //         SerialTxStr(str_iflags);
    //         SerialTxNum(p_sys->input_flags, DIGITS_3);

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         // DHW temperature
    //         SerialTxStr(str_lit_13);
    //         SerialTxNum(p_sys->dhw_temperature, DIGITS_4);
    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxNum(GetNtcTemperature(p_sys->dhw_temperature, TO_CELSIUS, DT_CELSIUS), DIGITS_3);

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         // CH temperature
    //         SerialTxStr(str_lit_14);
    //         SerialTxNum(p_sys->ch_temperature, DIGITS_4);
    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxNum(GetNtcTemperature(p_sys->ch_temperature, TO_CELSIUS, DT_CELSIUS), DIGITS_3);

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         // Overheat
    // #if !(OVERHEAT_OVERRIDE)
    //         SerialTxStr(str_lit_04);
    // #else
    //         SerialTxStr(str_lit_04_override);
    // #endif /* OVERHEAT_OVERRIDE */
    //         if (GetFlag(p_sys, INPUT_FLAGS, OVERHEAT)) {
    //             SerialTxStr(str_true);
    //         } else {
    //             SerialTxStr(str_false);
    //         }

    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */

    //         SerialTxStr(str_crlf); /* CR + new line */

    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */
    //         SerialTxChr(32);     /* Space (_) */

    //         // DHW Request
    //         SerialTxStr(str_lit_00);
    //         if (GetFlag(p_sys, INPUT_FLAGS, DHW_REQUEST)) {
    //             SerialTxStr(str_true);
    //         } else {
    //             SerialTxStr(str_false);
    //         }

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         //CH Request
    //         SerialTxStr(str_lit_01);
    //         if (GetFlag(p_sys, INPUT_FLAGS, CH_REQUEST)) {
    //             SerialTxStr(str_true);
    //         } else {
    //             SerialTxStr(str_false);
    //         }

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         // Airflow
    // #if !(AIRFLOW_OVERRIDE)
    //         SerialTxStr(str_lit_02);
    // #else
    //         SerialTxStr(str_lit_02_override);
    // #endif /* AIRFLOW_OVERRIDE */
    //         if (GetFlag(p_sys, INPUT_FLAGS, AIRFLOW)) {
    //             SerialTxStr(str_true);
    //         } else {
    //             SerialTxStr(str_false);
    //         }

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         // Flame
    //         SerialTxStr(str_lit_03);
    //         if (GetFlag(p_sys, INPUT_FLAGS, FLAME)) {
    //             SerialTxStr(str_true);
    //         } else {
    //             SerialTxStr(str_false);
    //         }

    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */

    //         SerialTxStr(str_crlf); /* CR + new line */

    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */
    //         SerialTxChr(32);     /* Space (_) */

    //         DrawLine(DASH_WIDTH - 4, H_ILINE); /* Dotted line */
    //         SerialTxChr(32);                   /* Space (_) */
    //         SerialTxChr(V_LINE);               /* Horizontal separator (|) */

    //         SerialTxStr(str_crlf); /* CR + new line */

    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */
    //         SerialTxChr(32);     /* Space (_) */

    //         SerialTxStr(str_lit_18);
    //         SerialTxChr(32); /* Space (_) */

    //         SerialTxStr(str_lit_15);
    //         SerialTxNum(p_sys->dhw_setting, DIGITS_4);
    //         SerialTxChr(32); /* Space (_) */
    //         //===SerialTxNum(GetHeatLevel(p_sys->dhw_setting, DHW_SETTING_STEPS), DIGITS_2);
    //         SerialTxNum(p_sys->dhw_heat_level, DIGITS_2);

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         SerialTxStr(str_lit_16);
    //         SerialTxNum(p_sys->ch_setting, DIGITS_4);
    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxNum(GetHeatLevel(p_sys->ch_setting, DHW_SETTING_STEPS), DIGITS_2);

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         SerialTxStr(str_lit_17);
    //         SerialTxNum(p_sys->system_setting, DIGITS_4);

    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */

    //         SerialTxStr(str_crlf); /* CR + new line */

    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */
    //         SerialTxChr(32);     /* Space (_) */

    //         DrawLine(DASH_WIDTH - 4, H_ILINE); /* Dotted line */
    //         SerialTxChr(32);
    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */

    //         SerialTxStr(str_crlf);

    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */
    //         SerialTxChr(32);     /* Space (_) */

    //         SerialTxStr(str_oflags);
    //         SerialTxNum(p_sys->output_flags, DIGITS_3);

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         // Exhaust fan
    //         SerialTxStr(str_lit_05);
    //         if (GetFlag(p_sys, OUTPUT_FLAGS, EXHAUST_FAN)) {
    //             SerialTxStr(str_true);
    //         } else {
    //             SerialTxStr(str_false);
    //         }

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         // Water pump
    //         SerialTxStr(str_lit_06);
    //         if (GetFlag(p_sys, OUTPUT_FLAGS, WATER_PUMP)) {
    //             SerialTxStr(str_true);
    //         } else {
    //             SerialTxStr(str_false);
    //         }

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         // Spark igniter
    //         SerialTxStr(str_lit_07);
    //         if (GetFlag(p_sys, OUTPUT_FLAGS, SPARK_IGNITER)) {
    //             SerialTxStr(str_true);
    //         } else {
    //             SerialTxStr(str_false);
    //         }

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         // LED UI
    //         SerialTxStr(str_lit_12);
    //         if (GetFlag(p_sys, OUTPUT_FLAGS, LED_UI)) {
    //             SerialTxStr(str_true);
    //         } else {
    //             SerialTxStr(str_false);
    //         }

    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */

    //         SerialTxStr(str_crlf); /* CR + new line */

    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */
    //         SerialTxChr(32);     /* Space (_) */

    //         // Security valve
    //         SerialTxStr(str_lit_08);
    //         if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_S)) {
    //             SerialTxStr(str_true);
    //         } else {
    //             SerialTxStr(str_false);
    //         }

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         // Valve 1
    //         SerialTxStr(str_lit_09);
    //         if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_1)) {
    //             SerialTxStr(str_true);
    //         } else {
    //             SerialTxStr(str_false);
    //         }

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         // Valve 2
    //         SerialTxStr(str_lit_10);
    //         if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_2)) {
    //             SerialTxStr(str_true);
    //         } else {
    //             SerialTxStr(str_false);
    //         }

    //         SerialTxChr(32); /* Space (_) */
    //         SerialTxChr(32); /* Space (_) */

    //         // Valve 3
    //         SerialTxStr(str_lit_11);
    //         if (GetFlag(p_sys, OUTPUT_FLAGS, VALVE_3)) {
    //             SerialTxStr(str_true);
    //         } else {
    //             SerialTxStr(str_false);
    //         }

    //         SerialTxChr(32);     /* Space (_) */
    //         SerialTxChr(V_LINE); /* Horizontal separator (|) */

    //         SerialTxStr(str_crlf);         /* CR + new line */
    //         DrawLine(DASH_WIDTH, H_ELINE); /* Dashed line */
    //         SerialTxStr(str_crlf);         /* CR + new line */

    // #if SHOW_PUMP_TIMER
    //         SerialTxStr(str_crlf);
    //         SerialTxStr(str_wptimer);
    //         SerialTxNum(p_sys->pump_delay, DIGITS_7);
    //         SerialTxStr(str_crlf);
    // #endif /* SHOW_PUMP_TIMER */
    //     }
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
