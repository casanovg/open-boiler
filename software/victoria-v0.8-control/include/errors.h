/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: errors.h (error codes header) for ATmega328
 *  ........................................................
 *  Version: 0.7 "Juan" / 2019-10-11 (News)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef _ERRORS_H_
#define _ERRORS_H_

// E000: No error
#define ERROR_000 0
// E001: Overheat thermostat open
#define ERROR_001 1
// E002: Flame sensor out of sequence
#define ERROR_002 2
// E003: Airflow sensor out of sequence
#define ERROR_003 3
// E004: Exhaust fan not producing enough airflow or airflow-sensor failure (before ignition)
#define ERROR_004 4
// E005: Ignition timeout, flame not detected or unexpectedly extinguished
#define ERROR_005 5
// E006: Airflow sensor didn't turn off on time
#define ERROR_006 6
// E007: No pressure detected in the flue-exhaust duct, there is a fan or airflow sensor failure (when the flame is lit)
#define ERROR_007 7
// E008: DHW sensor out of range: NTX thermistor failure or cable disconnected
#define ERROR_008 8
// E009: CH sensor out of range: NTX thermistor failure or cable disconnected
#define ERROR_009 9
// E010: Unexpected CH overtemperature
#define ERROR_010 10
// E011: Heat level inconsistency detected
#define ERROR_011 11

#endif /* _ERRORS_H_ */