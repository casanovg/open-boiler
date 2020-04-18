/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: victoria-control.c (main code) for ATmega328
 *  ........................................................
 *  Version: 0.8 "Juan" / 2019-04-09 ("Easter Quarantine")
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#include "victoria-control.h"

// Main function
int main(void) {
    /*  ___________________
       |                   | 
       |    Setup Block    |
       |___________________|
    */

    // Disable watch dog timer
    wdt_disable();

    // Initialize USART for serial communications (57600, N, 8, 1)
    SerialInit();

    // System gas modulator
    HeatModulator gas_modulator[] = {
        {VALVE_1, VALVE_1_F, 7000, 0.87, false},
        {VALVE_2, VALVE_2_F, 12000, 1.46, false},
        {VALVE_3, VALVE_3_F, 20000, 2.39, false}};

    //System state initialization
    SysInfo sys_info;
    SysInfo *p_system = &sys_info;
    p_system->system_mode = SYS_OFF;
    p_system->system_state = OFF;
    p_system->inner_step = OFF_1;
    p_system->input_flags = 0;
    p_system->output_flags = 0;
    p_system->last_displayed_iflags = 0;
    p_system->last_displayed_oflags = 0;
    p_system->error = ERROR_000;
    p_system->ignition_retries = 0;
    p_system->pump_delay = 0;
    p_system->ch_on_duty_step = CH_ON_DUTY_1;
    p_system->cycle_in_progress = 0;
    p_system->current_heat_level = 0;
    p_system->current_valve = 0;
    for (uint8_t valve = 0; valve < HEAT_MODULATOR_VALVES; valve++) {
        p_system->heat_modulator[valve] = gas_modulator[valve];
    }

    // Initialize ADC buffers
    AdcBuffers buffer_pack;
    AdcBuffers *p_buffer_pack = &buffer_pack;
    InitAdcBuffers(p_buffer_pack, BUFFER_LENGTH);

    // Initialize system flag byes
    InitFlags(p_system, INPUT_FLAGS);
    InitFlags(p_system, OUTPUT_FLAGS);

    // Initialize actuator controls
    for (OutputFlag device = EXHAUST_FAN_F; device <= LED_UI_F; device++) {
        InitActuator(p_system, device);
    }

    // Turn all actuators off
    for (OutputFlag device = EXHAUST_FAN_F; device <= LED_UI_F; device++) {
        ClearFlag(p_system, OUTPUT_FLAGS, device);
        _delay_ms(5);  // 5-millisecond blocking delay after turning each device off
    }

    // Initialize digital sensor flags
    for (InputFlag digital_sensor = DHW_REQUEST_F; digital_sensor <= OVERHEAT_F; digital_sensor++) {
        InitDigitalSensor(p_system, digital_sensor);
    }

    // Initialize analog sensor inputs
    for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
        InitAnalogSensor(p_system, analog_sensor);
    }

    // Pre-load analog sensor values
    for (uint8_t i = 0; i < BUFFER_LENGTH; i++) {
        for (AnalogInput analog_sensor = DHW_SETTING; analog_sensor <= CH_TEMPERATURE; analog_sensor++) {
            CheckAnalogSensor(p_system, p_buffer_pack, analog_sensor, false);
        }
    }

    // Show system operation status
    Dashboard(p_system, false);

    // WDT resets the system if it becomes unresponsive
    _delay_ms(2000);  // Safety 2-second blocking delay before activating the WDT
    //wdt_enable(WDTO_8S);  // If the system freezes, reset the microcontroller after 8 seconds

//#define VALVES 3 /* Number of gas modulator valves */

#define VALVE_3_TIMER_ID 1                   /* Timer id */
#define VALVE_3_TIMER_DURATION 500           /* Timer time-lapse */
#define VALVE_3_TIMER_MODE RUN_ONCE_AND_HOLD /* Timer mode */

    // Set system-wide timers
    SetTimer(VALVE_3_TIMER_ID, (unsigned long)VALVE_3_TIMER_DURATION, VALVE_3_TIMER_MODE); /* Water pump timer */

    // Enable global interrupts
    sei();
    SetTickTimer();

    /*  ___________________
      |                   | 
      |     Main Loop     |
      |___________________|
    */

    // #############################
    // #
    // # Force activating watch dog
    // #............................
    // # ONLY FOR BOOTLOADER TESTS!
    // #############################
    //for(;;) {};

    // Set timer for gas valve modulation
    SetTimer(VALVE_3_TIMER_ID, (unsigned long)VALVE_3_TIMER_DURATION, RUN_ONCE_AND_HOLD);

    for (;;) {

        if(TimerFinished(VALVE_3_TIMER_ID)) {
            
        }








#if SERIAL_DEBUG
        // // SerialTxStr(str_crlf);
        // SerialTxStr(str_heat_mod_01);
        // SerialTxNum(p_system->current_heat_level, DIGITS_2);
#endif

#if LED_DEBUG
        // SetFlag(p_system, OUTPUT_FLAGS, VALVE_S_F);  // Heat level setting error, the sum of the opening time of all valves must be 100!
        // _delay_ms(1000);
        // ClearFlag(p_system, OUTPUT_FLAGS, VALVE_S_F);
#endif

#if SERIAL_DEBUG
        // SerialTxStr(str_heat_mod_03);
#endif

#if SERIAL_DEBUG
        // SerialTxStr(str_heat_mod_02);
#endif

    } /* Main loop end */

    return 0;
}
