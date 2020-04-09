/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: delays.h (non-blocking delays) for ATmega328
 *  ........................................................
 *  Version: 0.7 "Juan" / 2019-10-11 (News)
 *  gustavo.casanova@nicebots.com
 *  ........................................................
 */

#ifndef DELAYS_H_
#define DELAYS_H_

// Long delay values: range -> 0 - 65535
#define DLY_L_OFF_2 100             /* Off_2 step long delay */
#define DLY_L_OFF_3 5000            /* Off_3 step long delay */
#define DLY_L_OFF_4 3000            /* Off_4 step long delay */
#define DLY_L_READY_1 3000          /* Ready_1 step long delay */
#define DLY_L_IGNITING_1 5          /* Igniting_1 step long delay */
#define DLY_L_IGNITING_2 5000       /* Igniting_2 step long delay */
#define DLY_L_IGNITING_3 200        /* Igniting_3 step long delay */
#define DLY_L_IGNITING_4 100        /* Igniting_4 step long delay */
#define DLY_L_IGNITING_5 300        /* Igniting_5 step long delay */
#define DLY_L_IGNITING_6 500        /* Igniting_6 step long delay */
#define DLY_L_DHW_ON_DUTY_1 100     /* On_DHW_Duty_1 step long delay */
#define DLY_L_DHW_ON_DUTY_LOOP 3000 /* On_DHW_Duty loop long delay */
#define DLY_L_CH_ON_DUTY_1 500      /* On_CH_Duty_1 step long delay */
#define DLY_L_CH_ON_DUTY_LOOP 3000  /* On_DHW_Duty loop long delay */
#define DLY_L_FLAME_MODULATION 9000 /* Modulation cycle: used in 1/3 parts */
#define DLY_WATER_PUMP_OFF 600000   /* Delay until the water pump shuts down when there are no CH requests */
                                    /* Time: 600000 / 60 / 1000 = 15 min aprox */
                                    /* Time: 900000 / 60 / 1000 = 15 min aprox */
                                    /* Time: 1800000 / 60 / 1000 = 30 min aprox     */
#define DLY_DEBOUNCE_CH_REQ 1000    /* Debounce delay for CH request thermostat switch */
#define DLY_DEBOUNCE_AIRFLOW 10     /* Debounce delay for airflow sensor switch */
#define DLY_FLAME_OFF 100           /* Delay before checking if the flame is off after closing gas */
#define DLY_AIRFLOW_OFF 2000        /* Delay before checking if the airflow sensor switches off when the fan gets turned off */

#endif /* DELAYS_H_ */