/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: victoria-control.h (headers) for ATmega328
 *  ........................................................
 *  Version: 0.7 "Juan" / 2019-10-11 (Easter Quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef _VICTORIA_CONTROL_H_
#define _VICTORIA_CONTROL_H_

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <hal.h>
#include <serial-ui.h>
#include <stdbool.h>
#include <timers.h>
#include <util/delay.h>

#include "errors.h"
#include "sys-settings.h"

// FSM non-blocking delay times (milliseconds)
#define DLY_OFF_2 10               // Off_2: Time before turning the fan for the flue exhaust test
#define DLY_OFF_3 5000             // Off_3: Time to let the fan to rev up and the airflow sensor closes (fan test)
#define DLY_OFF_4 3000             // Off_4: Time to let the fan to rev down after testing it (when the test is enabled)
#define DLY_READY_1 3000           // Ready_1: Time to allow the flame and airflow sensors to switch off after the gas is closed
#define DLY_IGNITING_1 10          // Igniting_1: Time-lapse from an ignition request until turning the fan on
#define DLY_IGNITING_2 5000        // Igniting_2: Time to let the fan to rev up and the airflow sensor closes (ignition)
#define DLY_IGNITING_3 250         // Igniting_3: Time before opening the security valve after the fan is running
#define DLY_IGNITING_4 125         // Igniting_4: Time before opening the valve 1 (or 2) after opening the security valve
#define DLY_IGNITING_5 125         // Igniting_5: Time before turning the spark igniter on after opening the valve 1 (or 2)
#define DLY_IGNITING_6 3000        // Igniting_6: Wait time for ignition with spark igniter and gas open before retrying
#define DLY_DHW_ON_DUTY_LOOP 3000  // DHW_on_Duty: Dashboard refreshing time when looping through DHW on-duty mode
#define DLY_CH_ON_DUTY_LOOP 3000   // CH_on_Duty: Dashboard refreshing time when looping through CH on-duty mode

#endif  // _VICTORIA_CONTROL_H_
