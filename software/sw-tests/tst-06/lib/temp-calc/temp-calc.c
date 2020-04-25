/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: temperature.c (temperature calculations library)
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-09 (Easter Quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#include "temp-calc.h"

// Function FIR filter
uint16_t FilterFir(uint16_t adc_buffer[], uint8_t buffer_length, uint8_t buffer_position) {
    unsigned long int aux = 0;
    uint16_t result;
    for (uint8_t i = 0; i < FIR_LEN; i++) {
        aux += (unsigned long)(adc_buffer[buffer_position++]) * (unsigned long)(fir_table[i]);
        if (buffer_position == buffer_length) {
            buffer_position = 0;
        }
    }
    result = ((unsigned long int)(aux)) / ((unsigned long int)(FIR_SUM));
    return result;
}

// Function IIR filter
uint16_t FilterIir(uint16_t adc_value) {
    static uint16_t Y;
    Y = (64 - IR_VAL) * adc_value + IR_VAL * Y;
    Y = (Y >> 6);
    return Y;
}

// Function CalculateNtcTemperature
int GetNtcTemperature(uint16_t ntc_adc_value, int temp_offset, int temp_delta) {
    int aux;
    uint16_t min, max;
    uint8_t i;
    // Search the table interval where the ADC value is located
    for (i = 0; (i < NTC_VALUES) && (ntc_adc_value < (ntc_adc_table[i])); i++)
        ;
    if ((i == 0) || (i == NTC_VALUES)) {  // If there is not located, return an error
        return -32767;
    }
    max = ntc_adc_table[i - 1];                 //Buscamos el valor más alto del intervalo
    min = ntc_adc_table[i];                     //y el más bajoa
    aux = (max - ntc_adc_value) * temp_delta;   //hacemos el primer paso de la interpolación
    aux = aux / (max - min);                    //y el segundo paso
    aux += (i - 1) * temp_delta + temp_offset;  //y añadimos el offset del resultado
    return aux;
}

// Function GetNtctempDegrees
float GetNtcTempDegrees(uint16_t ntc_adc_value, int temp_offset, int temp_delta) {
    int aux;
    uint16_t min, max;
    uint8_t i;
    // Search the table interval where the ADC value is located
    for (i = 0; (i < NTC_VALUES) && (ntc_adc_value < (ntc_adc_table[i])); i++)
        ;
    if ((i == 0) || (i == NTC_VALUES)) {  // If there is not located, return an error
        return -32767.0;
    }
    max = ntc_adc_table[i - 1];                 //Buscamos el valor más alto del intervalo
    min = ntc_adc_table[i];                     //y el más bajoa
    aux = (max - ntc_adc_value) * temp_delta;   //hacemos el primer paso de la interpolación
    aux = aux / (max - min);                    //y el segundo paso
    aux += (i - 1) * temp_delta + temp_offset;  //y añadimos el offset del resultado
    return ((float)aux / 10);
}
