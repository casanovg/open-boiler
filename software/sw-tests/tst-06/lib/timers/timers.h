/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: timers.h (system timers headers) for ATmega328
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-09 (Easter quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef _SYS_TIMERS_H_
#define _SYS_TIMERS_H_

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdbool.h>

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
#define DLY_L_OFF_2 100             /* Off_2 step long delay */
#define DLY_L_OFF_3 5000            /* Off_3 step long delay */
#define DLY_L_OFF_4 3000            /* Off_4 step long delay */
#define DLY_L_READY_1 3000          /* Ready_1 step long delay */
#define DLY_L_IGNITING_1 5          /* Igniting_1 step long delay */
#define DLY_L_IGNITING_2 5000       /* Igniting_2 step long delay */
#define DLY_L_IGNITING_3 200        /* Igniting_3 step long delay */
#define DLY_L_IGNITING_4 100        /* Igniting_4 step long delay */
#define DLY_L_IGNITING_5 300        /* Igniting_5 step long delay */
#define DLY_L_IGNITING_6 500        /* Igniting_6 step long delay */
#define DLY_L_DHW_ON_DUTY_1 100     /* On_DHW_Duty_1 step long delay */
#define DLY_L_DHW_ON_DUTY_LOOP 3000 /* On_DHW_Duty loop long delay */
#define DLY_L_CH_ON_DUTY_1 500      /* On_CH_Duty_1 step long delay */
#define DLY_L_CH_ON_DUTY_LOOP 3000  /* On_DHW_Duty loop long delay */
#define DLY_L_FLAME_MODULATION 9000 /* Modulation cycle: used in 1/3 parts */
#define DLY_WATER_PUMP_OFF 600000   /* Delay until the water pump shuts down when there are no CH requests */
                                    /* Time: 600000 / 60 / 1000 = 15 min aprox */
                                    /* Time: 900000 / 60 / 1000 = 15 min aprox */
                                    /* Time: 1800000 / 60 / 1000 = 30 min aprox     */
#define DLY_DEBOUNCE_CH_REQ 1000    /* Debounce delay for CH request thermostat switch */
#define DLY_DEBOUNCE_AIRFLOW 10     /* Debounce delay for airflow sensor switch */
#define DLY_FLAME_OFF 100           /* Delay before checking if the flame is off after closing gas */
#define DLY_AIRFLOW_OFF 2000        /* Delay before checking if the airflow sensor switches off when the fan gets turned off */

#define TIMER_BUFFER_SIZE 5
#define TIMER_EMPTY 0

typedef enum timer_mode {
    RUN_ONCE_AND_DELETE = 0,
    RUN_ONCE_AND_HOLD = 1,
    RUN_CONTINUOUSLY = 2  // Use this mode for call-back functions only!
} TimerMode;

typedef struct timers {
    uint8_t timer_id;
    unsigned long timer_start_time;
    unsigned long timer_time_lapse;
    TimerMode timer_mode;
} SystemTimers;

void SetTickTimer(void);
unsigned long GetMilliseconds(void);
uint8_t SetTimer(uint8_t, unsigned long, TimerMode);
bool TimerRunning(uint8_t);
bool TimerFinished(uint8_t);
void ProcessTimers();
void DeleteTimer(uint8_t);
uint8_t RestartTimer(uint8_t);
uint8_t ResetTimerLapse(uint8_t, unsigned long);
//void OnTimer(uint8_t);
//void OnTimer(SysInfo, uint8_t);

// Globals

// Timer function variables
static unsigned long timer0_overflow_cnt = 0;
static unsigned long timer0_milliseconds = 0;
static unsigned char timer0_fractions = 0;

// System Timers globals
SystemTimers timer_buffer[TIMER_BUFFER_SIZE];

#endif /* _SYS_TIMERS_H_ */
