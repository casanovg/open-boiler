/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: timers.h (system timers headers) for ATmega328
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-09 (Easter Quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef _SYS_TIMERS_H_
#define _SYS_TIMERS_H_

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdbool.h>

#include "../../include/sys-settings.h"

//Timer function defines

#define clockCyclesPerMicrosecond() (F_CPU / 1000000L)
#define clockCyclesToMicroseconds(a) (((a)*1000L) / (F_CPU / 1000L))
// #define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )
#define microsecondsToClockCycles(a) ((a)*clockCyclesPerMicrosecond())
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))
// The whole number of milliseconds per timer0 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)
// The fractional number of milliseconds per timer0 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

// Long delay values: range -> 0 - 65535
#define DLY_OFF_2 100             /* Off_2 step long delay */
#define DLY_OFF_3 5000            /* Off_3 step long delay */
#define DLY_OFF_4 3000            /* Off_4 step long delay */
#define DLY_READY_1 3000          /* Ready_1 step long delay */
#define DLY_IGNITING_1 5          /* Igniting_1 step long delay */
#define DLY_IGNITING_2 5000       /* Igniting_2 step long delay */
#define DLY_IGNITING_3 200        /* Igniting_3 step long delay */
#define DLY_IGNITING_4 100        /* Igniting_4 step long delay */
#define DLY_IGNITING_5 300        /* Igniting_5 step long delay */
#define DLY_IGNITING_6 500        /* Igniting_6 step long delay */
#define DLY_DHW_ON_DUTY_1 100     /* On_DHW_Duty_1 step long delay */
#define DLY_DHW_ON_DUTY_LOOP 3000 /* On_DHW_Duty loop long delay */
#define DLY_CH_ON_DUTY_1 500      /* On_CH_Duty_1 step long delay */
#define DLY_CH_ON_DUTY_LOOP 3000  /* On_DHW_Duty loop long delay */
#define DLY_FLAME_MODULATION 9000 /* Modulation cycle: used in 1/3 parts */
#define DLY_WATER_PUMP_OFF 600000 /* Delay until the water pump shuts down when there are no CH requests */
                                  /* Time: 300000 / 60 / 1000 = 5 min aprox */
                                  /* Time: 600000 / 60 / 1000 = 10 min aprox */
                                  /* Time: 900000 / 60 / 1000 = 15 min aprox */
                                  /* Time: 1800000 / 60 / 1000 = 30 min aprox     */
//#define DLY_DEBOUNCE_CH_REQ 1000    /* Debounce delay for CH request thermostat switch */
//#define DLY_DEBOUNCE_AIRFLOW 10     /* Debounce delay for airflow sensor switch */
#define DLY_FLAME_OFF 100    /* Delay before checking if the flame is off after closing gas */
#define DLY_AIRFLOW_OFF 2000 /* Delay before checking if the airflow sensor switches off when the fan gets turned off */

#define TIMER_EMPTY 0 /* Timer empty value */

// Types

typedef enum timer_mode {
    RUN_ONCE_AND_DELETE = 0,
    RUN_ONCE_AND_HOLD = 1,
    RUN_CONTINUOUSLY = 2  // Use this mode for call-back functions only!
} TimerMode;

typedef struct timer {
    uint8_t timer_id;
    uint32_t timer_start_time;
    uint32_t timer_time_lapse;
    TimerMode timer_mode;
} SystemTimer;

typedef uint8_t TimerId;
typedef uint32_t TimerLapse;

// Prototypes

uint8_t SetTimer(TimerId, TimerLapse, TimerMode);
bool TimerRunning(TimerId);
bool TimerFinished(TimerId);
bool TimerExists(TimerId);
uint32_t GetTimeLeft(TimerId);
uint8_t RestartTimer(TimerId);
uint8_t ResetTimerLapse(TimerId, TimerLapse);
void ProcessTimers();
bool CheckTimerExistence(TimerId);
void DeleteTimer(TimerId);
void SetTickTimer(void);
uint32_t GetMilliseconds(void);
//void OnTimer(uint8_t);
//void OnTimer(SysInfo, uint8_t);

// Globals

// Timer function variables
volatile static uint32_t timer0_milliseconds = 0; /* Range: 0 - 4294967295 milliseconds (49 days)*/
volatile static uint8_t timer0_fractions = 0;    /* Range 0 - 255 */
//volatile static unsigned long timer0_overflow_cnt = 0; /* Range 0 - 4294967295 */

// System Timers globals
SystemTimer timer_buffer[SYSTEM_TIMERS];

#endif /* _SYS_TIMERS_H_ */
