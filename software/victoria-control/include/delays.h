/*
 *  Open-Boiler Control - Victoria 20-20 T/F boiler control
 *  Author: Gustavo Casanova
 *  ........................................................
 *  File: delays.h (non-blocking delays) for ATmega328
 *  ........................................................
 *  Version: 0.5 "Juan" / 2019-08-19
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
#define DLY_L_DHW_ON_DUTY_1 500     /* On_DHW_Duty_1 step long delay */
#define DLY_L_DHW_ON_DUTY_LOOP 3000 /* On_DHW_Duty loop long delay */
#define DLY_L_CH_ON_DUTY_1 500      /* On_CH_Duty_1 step long delay */
#define DLY_L_CH_ON_DUTY_LOOP 3000  /* On_DHW_Duty loop long delay */
#define DLY_WATER_PUMP_OFF 1800000   /* Delay until the water pump shuts down when there are no CH requests */
                                    /* Time: 900000 / 60 / 1000 = 15 min aprox, 1800000 = 30 min aprox     */
#define DLY_DEBOUNCE 1000           /* Debounce delay for electromechanical switches */

#endif /* DELAYS_H_ */