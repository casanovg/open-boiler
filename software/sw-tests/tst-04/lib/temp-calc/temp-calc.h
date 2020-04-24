/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: temperature.h (temperature calculations headers)
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-09 (Easter quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef _TEMP_CALC_H_
#define _TEMP_CALC_H_

#include <avr/io.h>

#define ADC_MIN 0
#define ADC_MAX 1023
#define MAX_CH_TEMP_TOLERANCE 65

#define BUFFER_LENGTH 34 /* Circular buffers length */

#define CH_TEMP_MASK 0x3FE

// Filter settings
#define FIR_SUM 11872
#define IR_VAL 50
#define FIR_LEN 31

// Number of NTC ADC values used for calculating temperature
#define NTC_VALUES 12

// Temperature calculation settings
#define TO_CELSIUS -200   /* Celsius offset value */
#define DT_CELSIUS 100    /* Celsius delta T (difference between two consecutive table entries) */
#define TO_KELVIN 2530    /* Kelvin offset value */
#define DT_KELVIN 100     /* Kelvin delta T (difference between two consecutive table entries) */
#define TO_FAHRENHEIT -40 /* Fahrenheit offset value */
#define DT_FAHRENHEIT 180 /* Fahrenheit delta T (difference between two consecutive table entries) */

// Types
typedef enum average_type {
    MEAN = 0,
    ROBUST = 1,
    MOVING = 2
} AverageType;

typedef struct ring_buffer {
    uint16_t data[BUFFER_LENGTH];
    uint8_t ix;
} RingBuffer;

typedef struct adc_buffers {
    RingBuffer dhw_temp_adc_buffer;
    RingBuffer ch_temp_adc_buffer;
    RingBuffer dhw_set_adc_buffer;
    RingBuffer ch_set_adc_buffer;
    RingBuffer sys_set_adc_buffer;
} AdcBuffers;

// Prototypes
void InitAdcBuffers(AdcBuffers *, uint8_t);
uint16_t AverageAdc(uint16_t[], uint8_t, uint8_t, AverageType);
uint16_t FilterFir(uint16_t[], uint8_t, uint8_t);
uint16_t FilterIir(uint16_t);
int GetNtcTemperature(uint16_t, int, int);
int GetNtcTempTenths(uint16_t, int, int);
float GetNtcTempDegrees(uint16_t, int, int);
uint8_t GetHeatLevel(int16_t, uint8_t);

#define ADC_MIN 0
#define ADC_MAX 1023
#define MAX_CH_TEMP_TOLERANCE 65

#define CH_TEMP_MASK 0x3FE

// Filter settings
#define FIR_SUM 11872
#define IR_VAL 50
#define FIR_LEN 31

// Number of NTC ADC values used for calculating temperature
#define NTC_VALUES 12

// Temperature calculation settings
#define TO_CELSIUS -200   /* Celsius offset value */
#define DT_CELSIUS 100    /* Celsius delta T (difference between two consecutive table entries) */
#define TO_KELVIN 2530    /* Kelvin offset value */
#define DT_KELVIN 100     /* Kelvin delta T (difference between two consecutive table entries) */
#define TO_FAHRENHEIT -40 /* Fahrenheit offset value */
#define DT_FAHRENHEIT 180 /* Fahrenheit delta T (difference between two consecutive table entries) */

// Temperature to ADC readings conversion table
//  T°C:  -20, -10,   0,  10,  20,  30,  40,  50,  60,  70,  80, 90
//  ADC:  929, 869, 787, 685, 573, 461, 359, 274, 206, 154, 116, 87
//  NTC: 98.66, 56.25, 33.21, 20.24, 12.71, 8.19, 5.42, 3.66, 2.53, 1.78, 1.28, 0.93
static const uint16_t __flash ntc_adc_table[NTC_VALUES] = {
    929, 869, 787, 685, 573, 461, 359, 274, 206, 154, 116, 87};

// FIR filter value table
static const uint16_t __flash fir_table[FIR_LEN] = {
    1, 3, 9, 23, 48, 89, 149, 230, 333, 454, 586, 719, 840, 938, 1002, 1024,
    1002, 938, 840, 719, 586, 454, 333, 230, 149, 89, 48, 23, 9, 3, 1};

#endif /* _TEMP_CALC_H_ */
