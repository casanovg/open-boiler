/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: hw_mapping.h (hardware pin mapping) for ATmega328
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-19 (Easter Quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef _HARDWARE_MAPPING_H_
#define _HARDWARE_MAPPING_H_

// Flame detector (mini-pro pin 2 - input)
#define FLAME_DDR DDRD
#define FLAME_PIN PIN2
#define FLAME_PINP PIND
#define FLAME_PORT PORTD
// Exhaust fan (mini-pro pin 3 - output)
#define FAN_DDR DDRD
#define FAN_PIN PIN3
#define FAN_PINP PIND
#define FAN_PORT PORTD
// Spark igniter (mini-pro pin 4 - output)
#define SPARK_DDR DDRD
#define SPARK_PIN PIN4
#define SPARK_PINP PIND
#define SPARK_PORT PORTD
// Valve security (mini-pro pin 5 - output)
#define VALVE_S_DDR DDRD
#define VALVE_S_PIN PIN5
#define VALVE_S_PINP PIND
#define VALVE_S_PORT PORTD
// Valve 1 (mini-pro pin 6 - output)
#define VALVE_1_DDR DDRD
#define VALVE_1_PIN PIN6
#define VALVE_1_PINP PIND
#define VALVE_1_PORT PORTD
// Valve 2 (mini-pro pin 7 - output)
#define VALVE_2_DDR DDRD
#define VALVE_2_PIN PIN7
#define VALVE_2_PINP PIND
#define VALVE_2_PORT PORTD
// Valve 3 (mini-pro pin 8 - output)
#define VALVE_3_DDR DDRB
#define VALVE_3_PIN PIN0
#define VALVE_3_PINP PINB
#define VALVE_3_PORT PORTB
// Water pump (mini-pro pin 9 - output)
#define PUMP_DDR DDRB
#define PUMP_PIN PIN1
#define PUMP_PINP PINB
#define PUMP_PORT PORTB
// Overheat thermostat (mini-pro pin 10 - input)
#define OVERHEAT_DDR DDRB
#define OVERHEAT_PIN PIN2
#define OVERHEAT_PINP PINB
#define OVERHEAT_PORT PORTB
// Domestic Hot Water request (mini-pro pin 11 - input)
#define DHW_RQ_DDR DDRB
#define DHW_RQ_PIN PIN3
#define DHW_RQ_PINP PINB
#define DHW_RQ_PORT PORTB
// Central Heating request (mini-pro pin 12 - input)
#define CH_RQ_DDR DDRB
#define CH_RQ_PIN PIN4
#define CH_RQ_PINP PINB
#define CH_RQ_PORT PORTB
// Led UI (mini-pro pin 13 - onboard led - output)
#define LED_UI_DDR DDRB
#define LED_UI_PIN PIN5
#define LED_UI_PINP PINB
#define LED_UI_PORT PORTB
// DHW potentiometer (mini-pro pin A0 - intput)
#define DHW_POT ADC0
// CH potentiometer (mini-pro pin A1 - intput)
#define CH_POT ADC1
// System potentiometer (mini-pro pin A2 - intput)
#define SYS_POT ADC2
// Air-flow sensor (mini-pro pin A3 - intput) Board 4-6 left to right
#define AIRFLOW_DDR DDRC
#define AIRFLOW_PIN PIN3
#define AIRFLOW_PINP PINC
#define AIRFLOW_PORT PORTC
// System analog inputs ADC channels 
#define DHW_TEMP_ADC 6  // ADC6 - ADC Channel 6 (Pin A6)
#define CH_TEMP_ADC 7   // ADC7 - ADC Channel 7 (Pin A7)
#define DHW_POT_ADC 0   // ADC0 - ADC Channel 0 (Pin A0)
#define CH_SET_ADC 1    // ADC1 - ADC Channel 1 (Pin A1)
#define SYS_MOD_ADC 2   // ADC2 - ADC Channel 2 (Pin A2)

#endif  // _HARDWARE_MAPPING_H_
