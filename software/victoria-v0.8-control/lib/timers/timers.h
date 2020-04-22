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
#include <util/delay.h>

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

#define TIMER_EMPTY 0                  // Timer empty value
#define ENABLE_TIMERS_CALLBACKS false  // Sets if the ProcessTimers function, which makes the callbacks, will be run by the ISR

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

bool SetTimer(TimerId timer_id, TimerLapse time_lapse, TimerMode timer_mode);
bool TimerRunning(TimerId timer_id);
bool TimerFinished(TimerId timer_id);
bool TimerExists(TimerId timer_id);
uint32_t GetTimeLeft(TimerId timer_id);
uint8_t RestartTimer(TimerId timer_id);
uint8_t ResetTimerLapse(TimerId timer_id, uint32_t time_lapse);
void ProcessTimers(void);
void DeleteTimer(TimerId timer_id);
void SetTickTimer(void);
uint32_t GetMilliseconds(void);
//void OnTimer(uint8_t);
//void OnTimer(SysInfo, uint8_t);

// Globals

// Timer function variables
volatile static uint32_t timer0_milliseconds = 0;  // Range: 0 - 4294967295 milliseconds (49 days)
volatile static uint8_t timer0_fractions = 0;      // Range 0 - 255
//volatile static unsigned long timer0_overflow_cnt = 0; // Range 0 - 4294967295

// System Timers globals
SystemTimer timer_buffer[SYSTEM_TIMERS];

#endif  // _SYS_TIMERS_H_
