// ---------------------------------------------
// Test 01 - 2019-10-03 - Gustavo Casanova
// .............................................
// Heat modulation algorithm
// ---------------------------------------------

#include "tst-01.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

// Typedefs
typedef struct heat_level {
    uint8_t valve_open_time[3];
    uint16_t kcal_h;
    float gas_usage;
} HeatLevel;

typedef struct gas_valve {
    uint8_t valve_number;
    uint16_t kcal_h;
    float gas_usage;
    bool status;
} GasValve;

// Globals
HeatLevel heat_level[] = {
    /* { { %valve-1, %valve-3, %valve-3 }, Kcal/h, G20_m3 } */
    {{100, 0, 0}, 7000, 0.870},   /* Heat level 0 = 7000 Kcal/h */
    {{83, 17, 0}, 7833, 0.968},   /* Heat level 1 = 7833 Kcal/h */
    {{67, 33, 0}, 8667, 1.067},   /* Heat level 2 = 8667 Kcal/h */
    {{83, 0, 17}, 9167, 1.123},   /* Heat level 3 = 9167 Kcal/h */
    {{50, 50, 0}, 9500, 1.165},   /* Heat level 4 = 9500 Kcal/h */
    {{67, 17, 16}, 10000, 1.222}, /* Heat level 5 = 10000 Kcal/h */
    {{33, 67, 0}, 10333, 1.263},  /* Heat level 6 = 10333 Kcal/h */
    {{50, 33, 17}, 10833, 1.320}, /* Heat level 7 = 10833 Kcal/h */
    {{17, 83, 0}, 11167, 1.362},  /* Heat level 8 = 11167 Kcal/h */
    {{67, 0, 33}, 11333, 1.377},  /* Heat level 9 = 11333 Kcal/h */
    {{33, 50, 17}, 11667, 1.418}, /* Heat level 10 = 11667 Kcal/h */
    {{0, 100, 0}, 12000, 1.460},  /* Heat level 11 = 12000 Kcal/h */
    {{50, 17, 33}, 12167, 1.475}, /* Heat level 12 = 12167 Kcal/h */
    {{17, 67, 16}, 12500, 1.517}, /* Heat level 13 = 12500 Kcal/h */
    {{34, 33, 33}, 13000, 1.573}, /* Heat level 14 = 13000 Kcal/h */
    {{0, 83, 17}, 13333, 1.615},  /* Heat level 15 = 13333 Kcal/h */
    {{50, 0, 50}, 13500, 1.630},  /* Heat level 16 = 13500 Kcal/h */
    {{17, 50, 33}, 13833, 1.672}, /* Heat level 17 = 13833 Kcal/h */
    {{33, 17, 50}, 14333, 1.728}, /* Heat level 18 = 14333 Kcal/h */
    {{0, 67, 33}, 14667, 1.770},  /* Heat level 19 = 14667 Kcal/h */
    {{17, 33, 50}, 15167, 1.827}, /* Heat level 20 = 15167 Kcal/h */
    {{33, 0, 67}, 15667, 1.883},  /* Heat level 21 = 15667 Kcal/h */
    {{0, 50, 50}, 16000, 1.925},  /* Heat level 22 = 16000 Kcal/h */
    {{17, 17, 66}, 16500, 1.982}, /* Heat level 23 = 16500 Kcal/h */
    {{0, 33, 67}, 17333, 2.080},  /* Heat level 24 = 17333 Kcal/h */
    {{17, 0, 83}, 17833, 2.137},  /* Heat level 25 = 17833 Kcal/h */
    {{0, 17, 83}, 18667, 2.235},  /* Heat level 26 = 18667 Kcal/h */
    {{0, 0, 100}, 20000, 2.390}   /* Heat level 27 = 20000 Kcal/h */
};

GasValve gas_valve[] = {
    {1, 7000, 0.87, 0},
    {2, 12000, 1.46, 0},
    {3, 20000, 2.39, 0}
};

uint16_t cycle_time = 10000;
uint8_t cycle_slots = 6;
bool cycle_in_progress = 0;
uint8_t system_valves = (sizeof(gas_valve) / sizeof(gas_valve[0]));

// Prototypes
int main(void);
void Delay(unsigned int milli_seconds);

// Main function
int main(void) {
    uint8_t current_heat_level = 0; /* This level is determined by the CH temperature potentiometer */

    uint8_t current_valve = 0;
    unsigned long valve_open_timer = 0;

    #define BUFF_LEN 34

    uint16_t shake[BUFF_LEN];
    uint8_t length = BUFF_LEN;

    for (uint8_t i = 0; i < BUFF_LEN; i++) {
        shake[i] = i + 101;
    }

    printf("\r\nGC gas valve mixer\n\r");
    printf("==================\n\n\r");

    printf("Available heat levels:\n\r");
    printf("----------------------\n\r");
    for (int gas_level = 0; gas_level < sizeof(heat_level) / sizeof(heat_level[0]); gas_level++) {
        printf("Heat level %d: Kcal/h=%d G20-m3/h=%01.3f ", gas_level + 1, heat_level[gas_level].kcal_h, heat_level[gas_level].gas_usage);
        for (uint8_t v = system_valves; v > 0; v--) {
            printf("Valve-%d=%d ", v, heat_level[gas_level].valve_open_time[v - 1]);
        }
        printf("\n\r");
    }
    printf("\n\r");

    // Closing all valves
    for (uint8_t i = 0; i < system_valves; i++) {
        printf(" (X) Closing valve %d ...\n\r", i + 1);
        gas_valve[i].status = 0;
    }

    /*************
     * Main loop *
     *************
     */
    for (;;) {

        // if (length--) {
        //     // printf("\n\n\rSHAKE PRONTO SHAKE %d: %d\n\n\r", length, shake[length]);
        // } else {
        //     length = BUFF_LEN;
        //     printf("\n\n\r%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n\r");
        //     printf("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n\r");
        //     printf("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n\n\r");
        // }

        // ******************************************************************************************
        // Read the current heat level setup to determine what valves should be opened and for how long
        if (current_valve < system_valves) {
            //Valve-toggling cycle start
            if (cycle_in_progress == 0) {
                printf("\n\r============== >>> Cycle start: Heat level %d = %d Kcal/h... <<< ==============\n\r", current_heat_level + 1, heat_level[current_heat_level].kcal_h);
                uint8_t heat_level_time_usage = 0;
                int16_t cycle_slot_duration = cycle_time / cycle_slots;
                for (uint8_t vt_check = 0; vt_check < system_valves; vt_check++) {
                    heat_level_time_usage += heat_level[current_heat_level].valve_open_time[vt_check];
                }
                printf("\n\rCycle slot minimal duration: %d\n\r", cycle_slot_duration);
                printf("Heat level time usage: %d\n\n\r", heat_level_time_usage);
                if (heat_level_time_usage != 100) {
                    printf("<<< Heat level %d setting error, the sum of the opening time of all valves must be 100! >>>\n\r", current_heat_level + 1, heat_level_time_usage);
                    break;
                }
                cycle_in_progress = 1;
            }
            // If a heat-level cycle's current valve has an activation time setting other than zero ...
            if (heat_level[current_heat_level].valve_open_time[current_valve] > 0) {
                // If there are no valves open using a heat-level cycle time interval ...
                if (valve_open_timer == 0) {
                    // Set the valve opening time (delay) and open it ...
                    valve_open_timer = (heat_level[current_heat_level].valve_open_time[current_valve] * cycle_time / 100);
                    if (gas_valve[current_valve].status == 0) {
                        printf(" (O) Opening valve %d for %d ms!\n\r", current_valve + 1, valve_open_timer);
                        gas_valve[current_valve].status = 1; /* [ ] OPEN VALVE [ ] */
                    } else {
                        printf(" (=) Valve %d already open, keeping it as is for %d ms!\n\r", current_valve + 1, valve_open_timer);
                    }
                    // Close all other valves
                    for (uint8_t valve_to_close = 0; valve_to_close < system_valves; valve_to_close++) {
                        if (valve_to_close != current_valve) {
                            if (gas_valve[valve_to_close].status != 0) {
                                printf(" (X) Closing valve %d ...\n\r", valve_to_close + 1);
                                gas_valve[valve_to_close].status = 0; /* [x] CLOSE VALVE [x] */
                            }
                        }
                    }
                }
                
            } else {
                printf(" ||| Valve %d not set to be open in heat level %d (%d Kcal/h) ...\n\r", current_valve + 1, current_heat_level + 1, heat_level[current_heat_level].kcal_h);
                valve_open_timer++; /* This is necessary to move to the next valve */
            }
            // Open-valve delay
            if (!(--valve_open_timer)) {
                printf("\n\r");
                valve_open_timer = 0;
                current_valve++;
                // Valve-toggling cycle end
                if (current_valve >= system_valves) {
                    printf("============== >>> Cycle end: Heat level %d = %d Kcal/h... <<< ==============\n\n\r", current_heat_level + 1, heat_level[current_heat_level].kcal_h);
                    printf(" .......\n\r .......\n\r .......\n\r .......\n\r .......\n\r");
                    current_valve = 0;
                    cycle_in_progress = 0;
                    // Move to the next heat level
                    if (current_heat_level++ >= (sizeof(heat_level) / sizeof(heat_level[0])) - 1) {
                        current_heat_level = 0;
                        //break;
                    }
                }
            } else {
                Delay(1);
            }
        }
        // End of valves' toggling cycle
        // ******************************************************************************************
    }
    printf("\n\rLoop ended!\n\n\r");
}

// Function Delay
void Delay(unsigned int milli_seconds) {
    clock_t goal = milli_seconds + clock();
    while (goal > clock())
        ;
}
