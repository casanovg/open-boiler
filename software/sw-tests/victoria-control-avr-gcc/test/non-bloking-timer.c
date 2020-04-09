
/* set_timer function */
void SetTimer(unsigned int duration, char type) {
	char i;
	// Disable timers...
	//#asm(“cli”);
	for (i = 0; i < TIMER_BUFFER_SIZE; i++) { // loop through and check for free spot, then place timer in there...
		if (timer_queue[i].timer_id == TIMER_EMPTY) { // place new timer here...
			timer_queue[i].timer_id = type;
			timer_queue[i].end_time = duration + global_ms_timer;
		return;
		}
	}
	// Re-enable timers...
	//#asm("sei");
}

/* The process_timers is shown below. */

void ProcessTimers() {
	char i;
	for (i=0;i<TIMER_BUFFER_SIZE;i++) {
		if ((timer_queue[i].end_time == global_ms_timer) && (timer_queue[i].timer_id != TIMER_EMPTY)) {// put into message queue...
			on_timer(timer_queue[i].timer_id);
			timer_queue[i].timer_id = TIMER_EMPTY;
			timer_queue[i].end_time = TIMER_EMPTY;
		}
	}
}

/* The on_timer function (shown below) performs the actions on the Timer event. */

void OnTimer(char type) {
	switch (type) {
		case TURN_OFF_RX_LIGHT: {
			turn_off_rx_light();
			break;
		}
		case TURN_OFF_TX_LIGHT: {
			turn_off_tx_light();
			break;
		}
	}
}


/* The kill_timers function (shown below) */

void KillTimers(char type) {
    char i;
    // Disable timer interrupt
    //#asm("cli");
    for (i = 0; i < TIMER_BUFFER_SIZE; i++) { // loop through and check for timers of this type, then kill them...
        if (timer_queue[i].timer_id == type) { // kill timers...
            timer_queue[i].timer_id = TIMER_EMPTY;
            timer_queue[i].end_time = TIMER_EMPTY;
            return;
        }
    }
    // Enable Timer Interrupts
    //#asm("sei");
}

/*

Finally, we get down to what drives the Timer queue, and that’s the Timer Compare Interrupt. In my case, I have used the 8515 and the TIMER COMPARE A interrupt. I am running the chip at 8 MHz, and the clock is triggered at 125.00 kHz. The following code will set up the required values in the Output Compare Register A.

TCNT1H=0x00;
TCNT1L=0x00;
OCR1AH=0x00;
OCR1AL=0x7C;

Note that the value of the OCR1A Register is 124 (0x007C) as this corresponds to exactly 1 ms (0-124 = 125 triggers at 125kHz). The next step is to enable the Output Compare A interrupt.

TIMSK=0x40;

Once the global interrupts are enabled with the following code, the interrupt will occur exactly every 1 ms.

#asm(“sei”)

*/

// Timer 1 output compare A interrupt service routine
// Should occur every ms... 

interrupt [TIM1_COMPA] void timer1_compa_isr(void) { // Place your code here
    global_ms_timer++;
    ProcessTimers();
}







