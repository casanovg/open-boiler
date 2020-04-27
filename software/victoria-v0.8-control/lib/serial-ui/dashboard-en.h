/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: dashboard-en.h (English dashboard literals)
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-05-24 (Easter Quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef _DASHBOARD_EN_H_
#define _DASHBOARD_EN_H_

#include "../../include/sys-settings.h"

// Dashboard defines

#define FW_ALIAS "\"Easter Quarantine\"      "

#define DASH_WIDTH 66            // Dashboard horizontal line length
#define H_ELINE 46               // Horizonal external line characater (.)
#define H_ILINE 46               // Horizonal internal line characater (.)
#define V_LINE 46                // Vertical line characater (.)
#define DECIMAL_SEPARATOR 46     // Number decimal separator (.)
#define PUMP_TIMER_DIVISOR 1000  // Pump timer display units division (1000 = ms to s)
#define SPACE 32                 // Space ( )
#define TILDE 126                // ~
#define APOSTROPHE 39            // '
#define ASTERISK 42              // *
#define CHR_C 67                 // C
#define CHR_SQRB_O 91            // [
#define CHR_SQRB_C 93            // ]
#define CHR_RNDB_O 40            // (
#define CHR_RNDB_C 41            // )

#if SHOW_DASHBOARD
#define IGNITION_TRIES_CHR 84  // T
static const char __flash str_header_02[] = {FW_ALIAS};
static const char __flash str_space_xs[] = {" "};
static const char __flash str_space_s[] = {"  "};
static const char __flash str_space_m[] = {"   "};
static const char __flash str_space_l[] = {"    "};
static const char __flash str_space_xl[] = {"     "};
static const char __flash str_iflags[] = {"IF: "};
static const char __flash str_oflags[] = {"OF: "};
static const char __flash str_lit_00[] = {"DHW request: "};
static const char __flash str_lit_01[] = {"CH request: "};
static const char __flash str_lit_02[] = {" Airflow: "};
static const char __flash str_lit_02_override[] = {"!Airflow: "};
static const char __flash str_lit_03[] = {"Flame: "};
static const char __flash str_lit_04[] = {" Overheat: "};
static const char __flash str_lit_04_override[] = {"!Overheat: "};
static const char __flash str_lit_05[] = {"Fan: "};
static const char __flash str_lit_06[] = {"Pump: "};
static const char __flash str_lit_07[] = {"Igniter: "};
static const char __flash str_lit_08[] = {"Security valve: "};
static const char __flash str_lit_09[] = {" Valve-1: "};
static const char __flash str_lit_10[] = {"Valve-2: "};
static const char __flash str_lit_11[] = {"Valve-3: "};
static const char __flash str_lit_12[] = {"LED UI: "};
static const char __flash str_lit_13[] = {"DHW temp: "};
static const char __flash str_lit_14[] = {"CH temp: "};
static const char __flash str_lit_15[] = {"DHW set: "};
static const char __flash str_lit_16[] = {"CH set: "};
static const char __flash str_lit_17[] = {" Mode: "};
static const char __flash str_lit_18[] = {"Settings -> "};
static const char __flash str_true[] = {"Yes"};
static const char __flash str_false[] = {"No "};
static const char __flash str_error_s[] = {"                        >>> Error "};
static const char __flash str_error_e[] = {" <<<"};
static const char __flash sys_mode_00[] = {"[ WINTER ] "};
static const char __flash sys_mode_01[] = {"[ SUMMER ] "};
static const char __flash sys_mode_02[] = {"[ PWR-OFF ]"};
static const char __flash sys_mode_03[] = {"[ RESET ]  "};
static const char __flash str_mode_00[] = {"           [ OFF ] .\n\r"};
static const char __flash str_mode_10[] = {"         [ READY ] .\n\r"};
static const char __flash str_mode_20[] = {"      [ IGNITING ] .\n\r"};
static const char __flash str_mode_30[] = {"   [ DHW ON DUTY ] .\n\r"};
static const char __flash str_mode_40[] = {"  [ CH ON DUTY "};
static const char __flash str_mode_41[] = {"1 ] .\n\r"};
static const char __flash str_mode_42[] = {"2 ] .\n\r"};
static const char __flash str_mode_100[] = {"         [ ERROR ] .\n\r"};

#if SHOW_PUMP_TIMER
static const char __flash str_wptimer[] = {"  CH water pump shutdown timer: "};
static const char __flash str_wpmemory[] = {"<- Remaining time in memory: "};
static const char __flash str_temperr[] = {"XX.X"};
#endif  // SHOW_PUMP_TIMER

#else
static const char __flash str_no_dashboard[] = {"- System dashboard disabled in settings ..."};
#endif  // SHOW_DASHBOARD

#endif  // _DASHBOARD_EN_H_