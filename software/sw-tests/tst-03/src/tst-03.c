// ---------------------------------------------
// Test 03 - 2019-10-15 - Gustavo Casanova
// .............................................
// NTC resistance to ADC values to °C Temp
// ---------------------------------------------

#include "tst-03.h"

//#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

// ADC value range
#define ADC_MAX 1023
#define ADC_MIN 0

// ADC Buffer lenght
#define BUFF_LEN 34

// DHW setting steps
#define DHW_SETTING_STEPS 12

// Number of NTC ADC values used for calculating temperature
#define NTC_VALUES 12
#define PuntosTabla (12)

// NTC ADC temperature values
const uint16_t ntc_adc_table[NTC_VALUES] = {
    929, 869, 787, 685, 573, 461, 359, 274, 206, 154, 116, 87};

const unsigned int TablaADC[PuntosTabla] = {
    939, 892, 828, 749, 657, 560, 464, 377, 300, 237, 186, 55};
//-30,-20, -10,  0,   10,  20,  30,  40,  50,  60,  70 ºC
//  0   1    2   3     4    5    6    7    8    9   10  i

// Prototypes
int main(void);
void Delay(unsigned int milli_seconds);
int GetNtcTempTenths(uint16_t, int, int);
float GetNtcTempDegrees(uint16_t, int, int);
int TempNTC(unsigned int, int, int);
uint8_t GetHeatLevel(int16_t, uint8_t);
int DivRound(const int numerator, const int denominator);

// Temperature calculation settings
#define TO_CELSIUS (-200)    /* Celsius offset value */
#define DT_CELSIUS (100)     /* Celsius delta T (difference between two consecutive table entries) */
#define TO_KELVIN (2430)     /* Kelvin offset value */
#define DT_KELVIN (100)      /* Kelvin delta T (difference between two consecutive table entries) */
#define TO_FAHRENHEIT (-220) /* Fahrenheit offset value */
#define DT_FAHRENHEIT (180)  /* Fahrenheit delta T (difference between two consecutive table entries) */

//Parámetros para conversión en ºCelsius
#define ToCels (-300)
#define dTCels (100)
//Parámetros para conversión en Kelvins
#define ToKelv (2430)
#define dTKelv (100)
//Parámetros para conversión en ºFahrenheit
#define ToFahr (-220)
#define dTFahr (180)

// Main function
int main(void) {
    uint8_t current_heat_level = 0; /* This level is determined by the CH temperature potentiometer */

    uint8_t current_valve = 0;
    unsigned long valve_open_timer = 0;

    uint16_t shake[BUFF_LEN];
    uint8_t length = BUFF_LEN;

    for (uint8_t i = 0; i < BUFF_LEN; i++) {
        shake[i] = i + 101;
    }

    printf("\r\nGC temperature calculation\n\r");
    printf("==========================\n\n\r");

    const uint16_t dhw_pot = 241;

    //const uint16_t adc_temp = 347;

    //printf("\n\rADC B: %d, Temperature calculation = %d\n\n\r", adc_temp, TempNTC(adc_temp, TO_CELSIUS, DT_CELSIUS));
    for (uint16_t adc_temp = 1023; adc_temp > 0; adc_temp--) {
        int celsius_tenths = GetNtcTempTenths(adc_temp, TO_CELSIUS, DT_CELSIUS);
        float celsius_degrees = GetNtcTempDegrees(adc_temp, TO_CELSIUS, DT_CELSIUS);
        if (celsius_degrees != -32767) {
            // int celsius_grades = celsius_centigrades / 10;
            // //int celsius_decimals = celsius_centigrades - (celsius_grades * 10);
            // int celsius_decimals = celsius_centigrades % 10;
            // if (celsius_decimals < 0) {
            //     celsius_decimals *= -1;
            // }
            // printf("ADC B: %d, Temp Value = %d, Celsius calculation = %2d.%1d\n\r",
            //        adc_temp, celsius_centigrades, celsius_grades, celsius_decimals);
            printf("ADC output: %d, Temp value = %d, Celsius calculation = %.1f \370C -> Float sim: %02u.%1u\n\r",
                   adc_temp,
                   celsius_tenths,
                   celsius_degrees,
                   //DivRound((uint16_t)celsius_tenths, 10),
                   ((uint16_t)celsius_tenths) / 10,
                   (uint16_t)celsius_tenths % 10);
        }
    }

    printf("\n\n\rDHW pot adc = %d, DHW heat level setting = %d\n\r", dhw_pot, GetHeatLevel(dhw_pot, DHW_SETTING_STEPS));

    printf("\n\n\r");
}

// Function Delay
void Delay(unsigned int milli_seconds) {
    clock_t goal = milli_seconds + clock();
    while (goal > clock())
        ;
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

// Function TempNTC
int TempNTC(unsigned int adc, int To, int dT) {
    int aux_;
    unsigned int min_, max_;
    unsigned char i_;

    //Buscamos el intervalo de la tabla en que se encuentra el valor de ADC
    for (i_ = 0; (i_ < PuntosTabla) && (adc < (TablaADC[i_])); i_++)
        ;
    if ((i_ == 0) || (i_ == PuntosTabla))  //Si no está, devolvemos un error
        return -32767;
    max_ = TablaADC[i_ - 1];      //Buscamos el valor más alto del intervalo
    min_ = TablaADC[i_];          //y el más bajoa
    aux_ = (max_ - adc) * dT;     //hacemos el primer paso de la interpolación
    aux_ = aux_ / (max_ - min_);  //y el segundo paso
    aux_ += (i_ - 1) * dT + To;   //y añadimos el offset del resultado
    return aux_;
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

// Function DivRound
int DivRound(const int numerator, const int denominator) {
    int result = 0;
    if ((numerator < 0) ^ (denominator < 0)) {
        result = ((numerator - denominator / 2) / denominator);
    } else {
        result = ((numerator + denominator / 2) / denominator);
    }
    return result;
}
