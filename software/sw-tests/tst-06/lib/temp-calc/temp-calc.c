/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: temperature.c (temperature calculations library)
 *  ........................................................
 *  Version: 0.8 "Juan" / 2020-04-09 (Easter quarantine)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#include "temp-calc.h"

// Function InitAdcBuffers
void InitAdcBuffers(AdcBuffers *p_buffer_pack, uint8_t buffer_length) {
    p_buffer_pack->dhw_temp_adc_buffer.ix = 0;
    p_buffer_pack->ch_temp_adc_buffer.ix = 0;
    p_buffer_pack->dhw_set_adc_buffer.ix = 0;
    p_buffer_pack->ch_set_adc_buffer.ix = 0;
    p_buffer_pack->sys_set_adc_buffer.ix = 0;
    for (uint8_t i = 0; i < buffer_length; i++) {
        p_buffer_pack->dhw_temp_adc_buffer.data[i] = 0;
        p_buffer_pack->ch_temp_adc_buffer.data[i] = 0;
        p_buffer_pack->dhw_set_adc_buffer.data[i] = 0;
        p_buffer_pack->ch_set_adc_buffer.data[i] = 0;
        p_buffer_pack->sys_set_adc_buffer.data[i] = 0;
    }
}

// Function AverageAdc
uint16_t AverageAdc(uint16_t adc_buffer[], uint8_t buffer_len, uint8_t start, AverageType average_type) {
    uint16_t avg_value = 0;
    switch (average_type) {
        case MEAN: {
            for (uint8_t i = 0; i < buffer_len; i++) {
                avg_value += adc_buffer[i];
            }
            avg_value = avg_value / buffer_len;
            break;
        }
        case ROBUST: {
            uint16_t max, min;
            avg_value = max = min = adc_buffer[0];
            for (uint8_t i = 1; i < buffer_len; i++) {
                avg_value += adc_buffer[i];
                if (adc_buffer[i] > max) {
                    max = adc_buffer[i];
                } else if (adc_buffer[i] < min) {
                    min = adc_buffer[i];
                }
            }
            avg_value -= max;
            avg_value -= min;
            //avg_value = (avg_value >> 5);
            avg_value = avg_value / (buffer_len - 2);
            break;
        }
        case MOVING: {
            avg_value += adc_buffer[start];
            if (start == (buffer_len - 1)) {
                avg_value -= adc_buffer[0];
            } else {
                avg_value -= adc_buffer[start + 1];
            }
            break;
        }
        default: {
            break;
        }
    }
    return avg_value;
}

// Function FIR filter
uint16_t FilterFir(uint16_t adc_buffer[], uint8_t buffer_len, uint8_t start) {
    unsigned long int aux = 0;
    uint16_t result;
    for (uint8_t i = 0; i < FIR_LEN; i++) {
        aux += (unsigned long)(adc_buffer[start++]) * (unsigned long)(fir_table[i]);
        if (start == buffer_len) {
            start = 0;
        }
    }
    result = ((unsigned long int)(aux)) / ((unsigned long int)(FIR_SUM));
    return result;
}

// Function IIR filter
uint16_t FilterIir(uint16_t value) {
    static uint16_t Y;
    Y = (64 - IR_VAL) * value + IR_VAL * Y;
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

// Function GetNtcTempTenths
int GetNtcTempTenths(uint16_t ntc_adc_value, int temp_offset, int temp_delta) {
    int aux;
    uint16_t min, max;
    uint8_t i;
    // Search the table interval where the ADC value is located
    for (i = 0; (i < NTC_VALUES) && (ntc_adc_value < (ntc_adc_table[i])); i++)
        ;
    if ((i == 0) || (i == NTC_VALUES)) {  // If there is not located, return an error
        return -32767;
    }
    //printf("ADC table entry (%d) = %d\n\r", i, ntc_adc_table[i]);
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

// Function GetHeatLevel
uint8_t GetHeatLevel(int16_t pot_adc_value, uint8_t knob_steps) {
    uint8_t heat_level = 0;
    for (heat_level = 0; (pot_adc_value < (ADC_MAX - ((ADC_MAX / knob_steps) * (heat_level + 1)))); heat_level++)
        ;
    if (heat_level >= knob_steps) {
        heat_level = --knob_steps;
    }
    return heat_level;
}
