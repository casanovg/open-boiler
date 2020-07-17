/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: dashboard-es.h (Spanish dashboard literals)
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-05-24 (Easter Quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef DASHBOARD_ES_H
#define DASHBOARD_ES_H

#include "../../include/sys-settings.h"

// Dashboard defines

#define FW_ALIAS "\"Cuarentena de Pascua\"   "

#define DASH_WIDTH 70            // Dashboard horizontal line length
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
#define IGNITION_TRIES_CHR 73  // I
static const char __flash str_header_02[] = {FW_ALIAS};
static const char __flash str_space_xs[] = {" "};
static const char __flash str_space_s[] = {"  "};
static const char __flash str_space_m[] = {"   "};
static const char __flash str_space_l[] = {"    "};
static const char __flash str_space_xl[] = {"     "};
static const char __flash str_iflags[] = {"Ent: "};
static const char __flash str_oflags[] = {"Sal: "};
static const char __flash str_lit_00[] = {"Servicio ACS: "};
static const char __flash str_lit_01[] = {"Servicio CC: "};
static const char __flash str_lit_02[] = {" Sensor aire: "};
static const char __flash str_lit_02_override[] = {"!Sensor aire: "};
static const char __flash str_lit_03[] = {"  LLama: "};
static const char __flash str_lit_04[] = {"   Termostato: "};
static const char __flash str_lit_04_override[] = {"  !Termostato: "};
static const char __flash str_lit_05[] = {"Extractor: "};
static const char __flash str_lit_06[] = {"Bomba: "};
static const char __flash str_lit_07[] = {"Encendedor: "};
static const char __flash str_lit_08[] = {"Valvula seguridad: "};
static const char __flash str_lit_09[] = {"Valvula-1: "};
static const char __flash str_lit_10[] = {"Valvula-2: "};
static const char __flash str_lit_11[] = {"Valvula-3: "};
static const char __flash str_lit_12[] = {"LED: "};
static const char __flash str_lit_13[] = {"Temp ACS: "};
static const char __flash str_lit_14[] = {"Temp CC: "};
static const char __flash str_lit_15[] = {" Sel ACS: "};
static const char __flash str_lit_16[] = {" Sel CC: "};
static const char __flash str_lit_17[] = {" Modo:"};
static const char __flash str_lit_18[] = {"Controles ->"};
static const char __flash str_true[] = {"Si"};
static const char __flash str_false[] = {"No"};
static const char __flash str_error_s[] = {"                        >>> Error "};
static const char __flash str_error_e[] = {" <<<"};
static const char __flash sys_mode_00[] = {" [ INVIERNO ] "};
static const char __flash sys_mode_01[] = {" [ VERANO ]   "};
static const char __flash sys_mode_02[] = {" [ APAGADO ]  "};
static const char __flash sys_mode_03[] = {" [ REINICIO ] "};
static const char __flash str_mode_00[] = {"          [ INACTIVO ] .\n\r"};
static const char __flash str_mode_10[] = {"             [ LISTO ] .\n\r"};
static const char __flash str_mode_20[] = {"       [ ENCENDIENDO ] .\n\r"};
static const char __flash str_mode_30[] = {"   [ ACS EN SERVICIO ] .\n\r"};
static const char __flash str_mode_40[] = {"  [ CC EN SERVICIO "};
static const char __flash str_mode_41[] = {"1 ] .\n\r"};
static const char __flash str_mode_42[] = {"2 ] .\n\r"};
static const char __flash str_mode_100[] = {"             [ ERROR ] .\n\r"};

#if SHOW_PUMP_TIMER
static const char __flash str_wptimer[] = {"  Temporizador apagado bomba CC: "};
static const char __flash str_wpmemory[] = {"<- Tiempo restante memorizado: "};
static const char __flash str_temperr[] = {"XX.X"};
#endif  // SHOW_PUMP_TIMER

#else
static const char __flash str_no_dashboard[] = {"- Tablero del sistema desabilitado en consiguracion ..."};
#endif  // SHOW_DASHBOARD

#endif  // DASHBOARD_ES_H