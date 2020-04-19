/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: timers.c (system timers library) for ATmega328
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-09 (Easter Quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#include "timers.h"

// Function SetTimer
uint8_t SetTimer(TimerId timer_id, TimerLapse time_lapse, TimerMode timer_mode) {
    // Disable timers..
    //cli();
    for (uint8_t i = 0; i < SYSTEM_TIMERS; i++) {   // loop through and check for free spot, then place timer in there...
        if (timer_buffer[i].timer_id == TIMER_EMPTY) {  // place new timer here...
            timer_buffer[i].timer_id = timer_id;
            timer_buffer[i].timer_start_time = GetMilliseconds();
            timer_buffer[i].timer_time_lapse = time_lapse;
            timer_buffer[i].timer_mode = timer_mode;
            return i;
        }
    }
    // Re-enable timers..
    //sei();
    return 0;
}

// Function TimerRunning
bool TimerRunning(TimerId timer_id) {
    for (uint8_t i = 0; i < SYSTEM_TIMERS; i++) {
        if (timer_buffer[i].timer_id == timer_id) {
            if ((GetMilliseconds() - timer_buffer[i].timer_start_time) >= timer_buffer[i].timer_time_lapse) {
                return false;
            }
        }
    }
    return true;
}

// Function TimerFinished
bool TimerFinished(TimerId timer_id) {
    for (uint8_t i = 0; i < SYSTEM_TIMERS; i++) {
        if (timer_buffer[i].timer_id == timer_id) {
            if ((GetMilliseconds() - timer_buffer[i].timer_start_time) >= timer_buffer[i].timer_time_lapse) {
                return true;
            }
        }
    }
    return false;
}

// Function CheckTimerExistence
bool TimerExists(TimerId timer_id) {
    for (uint8_t i = 0; i < SYSTEM_TIMERS; i++) {
        if (timer_buffer[i].timer_id == timer_id) {
            return true;
        }
    }
    return false;
}

// Function GetTimeLeft
uint32_t GetTimeLeft(TimerId timer_id) {
    uint32_t time_left = 0;
    for (uint8_t i = 0; i < SYSTEM_TIMERS; i++) {
        if (timer_buffer[i].timer_id == timer_id) {
            time_left = timer_buffer[i].timer_start_time + timer_buffer[i].timer_time_lapse - GetMilliseconds();
            if (time_left > timer_buffer[i].timer_time_lapse) {
                time_left = 0;
            }

        }
    }
    return time_left;
}

// Function RestartTimer
uint8_t RestartTimer(TimerId timer_id) {
    for (uint8_t i = 0; i < SYSTEM_TIMERS; i++) {
        if (timer_buffer[i].timer_id == timer_id) {
            if (timer_buffer[i].timer_mode == RUN_ONCE_AND_HOLD) {  //&&
                //(TimerRunning(i) == false)) {
                timer_buffer[i].timer_start_time = GetMilliseconds();
                return 0;
            } else {
                return 255; /* Error: The timer is empty or its type doesn't allow restarts or is running */
            }
        }
    }
    return 0;
}

// Function ResetTimerLapse
uint8_t ResetTimerLapse(TimerId timer_id, uint32_t time_lapse) {
    for (uint8_t i = 0; i < SYSTEM_TIMERS; i++) {
        if (timer_buffer[i].timer_id == timer_id) {
            if (timer_buffer[i].timer_mode == RUN_ONCE_AND_HOLD) {  //&&
                //(TimerRunning(i) == false)) {
                timer_buffer[i].timer_start_time = GetMilliseconds();
                timer_buffer[i].timer_time_lapse = time_lapse;
                return 0;
            } else {
                return 255; /* Error: The timer is empty or its type doesn't allow restarts or is running */
            }
        }
    }
    return 0;
}

// Function ProcessTimers
void ProcessTimers() {
    for (uint8_t i = 0; i < SYSTEM_TIMERS; i++) {
        if ((GetMilliseconds() - timer_buffer[i].timer_start_time >= timer_buffer[i].timer_time_lapse) &&
            (timer_buffer[i].timer_id != TIMER_EMPTY)) {
            switch (timer_buffer[i].timer_mode) {
                case RUN_ONCE_AND_HOLD: {
                    break;
                }
                case RUN_CONTINUOUSLY: {
                    timer_buffer[i].timer_start_time = GetMilliseconds();
                    break;
                }
                default: { /* RUN_ONCE_AND_DELETE */
                    timer_buffer[i].timer_id = TIMER_EMPTY;
                    timer_buffer[i].timer_start_time = 0;
                    timer_buffer[i].timer_time_lapse = 0;
                    break;
                }
            }
            //OnTimer(timer_buffer[i].timer_id);
        }
    }
}

// Function DeleteTimers (Deletes all timers of the same type)
void DeleteTimer(TimerId timer_id) {
    // Disable timer interrupt
    //#asm("cli");
    for (uint8_t i = 0; i < SYSTEM_TIMERS; i++) {  // loop through and check for timers of this type, then kill them...
        if (timer_buffer[i].timer_id == timer_id) {    // kill timers...
            timer_buffer[i].timer_id = TIMER_EMPTY;
            timer_buffer[i].timer_start_time = 0;
            timer_buffer[i].timer_time_lapse = 0;
            return;
        }
    }
    // Enable Timer Interrupts
    //#asm("sei");
}

// Function SetTickTimer: Sets the Timer0 hardware up
void SetTickTimer(void) {
    // Set prescaler factor 64
    TCCR0B |= (1 << CS00);
    TCCR0B |= (1 << CS01);
    // Enable timer 0 overflow interrupt
    TIMSK0 |= (1 << TOIE0);
}

// Function GetMilliseconds: Returns the milliseconds that passed since the last counter overflow (every 49 days)
uint32_t GetMilliseconds(void) {
    uint32_t m;
    uint8_t oldSREG = SREG;
    // disable interrupts while we read timer0_milliseconds or we might get an
    // inconsistent value (e.g. in the middle of a write to timer0_millis)
    cli();
    m = timer0_milliseconds;
    SREG = oldSREG;
    return m;
}

// Timer 0 overflow interrupt service routine
ISR(TIMER0_OVF_vect) {
    // copy these to local variables so they can be stored in registers
    // (volatile variables must be read from memory on every access)
    uint32_t m = timer0_milliseconds;
    uint8_t f = timer0_fractions;

    m += MILLIS_INC;
    f += FRACT_INC;
    if (f >= FRACT_MAX) {
        f -= FRACT_MAX;
        m += 1;
    }

    timer0_fractions = f;
    timer0_milliseconds = m;
    //timer0_overflow_cnt++;

    //ProcessTimers();
}
