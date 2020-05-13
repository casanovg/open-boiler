/*
 *  NB interrupt-based TWI-hardware driver
 *  Author: Gustavo Casanova
 *  .............................................
 *  File: nb-twisl.c (Slave driver library)
 *  .............................................
 *  Version: 0.9.0 / 2020-05-12
 *  gustavo.casanova@nicebots.com
 *  .............................................
 *  Based on works by Atmel (AVR311) et others
 *  .............................................
 */

#include "nb-twisl.h"

#include <avr/interrupt.h>
#include <avr/io.h>

// USI TWI driver globals
static uint8_t rx_buffer[TWI_RX_BUFFER_SIZE];
static uint8_t tx_buffer[TWI_TX_BUFFER_SIZE];
static volatile uint8_t rx_head;
static volatile uint8_t rx_tail;
static volatile uint8_t tx_head;
static volatile uint8_t tx_tail;

/*  ___________________
   |                   |
   | Flush TWI buffers |
   |___________________|
*/
void FlushTwiBuffers(void) {
    rx_head = 0, rx_tail = 0;
    tx_head = 0, tx_tail = 0;
}

/*  ___________________________
   |                           |
   | TWI driver initialization |
   |___________________________|
*/
void TwiDriverInit(uint8_t address) {
    TWAR = (address << 1) | (0);                        // Device TWI address. Accept TWI general calls
    TWCR = (1 << TWEN) |                                // Enable TWI interface and release TWI pins
           (0 << TWIE) | (0 << TWINT) |                 // Disable TWI Interrupt
           (0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) |  // Do not ACK on any requests, yet
           (0 << TWWC);                                 //
    return;
}

/*  ___________________
   |                   |
   | TWI driver enable |
   |___________________|
*/
void TwiDriverEnable(void) {
    TWCR = (1 << TWEN) |                                // TWI interface enabled
           (1 << TWIE) | (1 << TWINT) |                 // Enable TWI interrupt and clear the flag
           (1 << TWEA) | (0 << TWSTA) | (0 << TWSTO) |  // Prepare to ACK next time the slave is addressed
           (0 << TWWC);                                 //
}

/*  _______________________
   |                       |
   | TWI byte transmission |
   |_______________________|
*/
void TwiTransmitByte(uint8_t data_byte) {
    uint8_t tmphead;
    // calculate buffer index
    tmphead = (tx_head + 1) & TWI_TX_BUFFER_MASK;
    // check for free space in buffer
    if (tmphead == tx_tail) {
        return;
    }
    // store data into buffer
    tx_buffer[tmphead] = data_byte;
    // update index
    tx_head = tmphead;
}

/*  ____________________
   |                    |
   | TWI byte reception |
   |____________________|
*/
uint8_t TwiReceiveByte(void) {
    // check for available data.
    if (rx_head == rx_tail) {
        return 0x88;
    }
    // generate index
    rx_tail = (rx_tail + 1) & TWI_RX_BUFFER_MASK;
    return rx_buffer[rx_tail];
}

/*  __________________________
   |                          |
   | Check TWI receive buffer |
   |__________________________|
*/
bool TwiCheckReceiveBuffer(void) {
    // return 0 (false) if the receive buffer is empty
    return rx_head != rx_tail;
}

/*  ___________________________
   |                           |
   | Clear TWI transmit buffer |
   |___________________________|
*/
void TwiClearOutputBuffer(void) {
    tx_tail = 0;
    tx_head = 0;
}

/*  _________________________
   |                         |
   | TWI fill receive buffer |
   |_________________________|
*/
void TwiFillReceiveBuffer(uint8_t data) {
    uint8_t tmphead;
    // calculate buffer index
    tmphead = (rx_head + 1) & TWI_RX_BUFFER_MASK;
    // check for free space in buffer
    if (tmphead == rx_tail) {
        return;
    }
    // store data into buffer
    rx_buffer[tmphead] = data;
    // update index
    rx_head = tmphead;
}

/*  ______________________________________
   |                                      |
   | TWI device interrupt service routine |
   |______________________________________|
*/
ISR(TWI_vect) {
    switch (TWSR) {
        case TWI_SRX_ADR_ACK: {  // Own SLA+R has been received; ACK has been returned
            TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
            break;
        }
        case TWI_SRX_ADR_DATA_ACK:
        case TWI_SRX_GEN_DATA_ACK: {
            // Fill receive buffer
            TwiFillReceiveBuffer(TWDR);
            TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
            break;
        }
        case TWI_SRX_GEN_ACK: {
            // Prepare for next event
            TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
            break;
        }
        case TWI_STX_ADR_ACK:
        case TWI_STX_DATA_ACK: {
            if (tx_head != tx_tail) {
                tx_tail = (tx_tail + 1) & TWI_TX_BUFFER_MASK;
                TWDR = tx_buffer[tx_tail];
            } else {
                // Buffer empty
                TWDR = 0x88;
            }
            // Prepare for next event
            TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
            break;
        }
        case TWI_STX_DATA_NACK: {
            // Prepare for next event
            TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
            break;
        }
        case TWI_SRX_STOP_RESTART: {
            // Prepare for next event
            TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);

            // SEE HERE *** SEE HERE *** SEE HERE *** SEE HERE *** SEE HERE

            break;
        }
        case TWI_SRX_ADR_DATA_NACK:
        case TWI_SRX_GEN_DATA_NACK:
        case TWI_STX_DATA_ACK_LAST_BYTE:
        case TWI_NO_STATE:
        case TWI_BUS_ERROR: {
            // TWI bus error
            TWCR = (1 << TWSTO) | (1 << TWINT);
            break;
        }
        default: {
            // Prepare for next event
            TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
            break;
        }
    }
}
