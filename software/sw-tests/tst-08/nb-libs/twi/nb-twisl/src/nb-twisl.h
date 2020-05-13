/*
 *  NB interrupt-based TWI-hardware driver
 *  Author: Gustavo Casanova
 *  .............................................
 *  File: nb-twisl.h (Slave driver headers)
 *  .............................................
 *  Version: 0.9.0 / 2020-05-12
 *  gustavo.casanova@nicebots.com
 *  .............................................
 *  Based on works by Atmel (AVR311) et others
 *  .............................................
 */

#ifndef TWISLAVE_H_
#define TWISLAVE_H_

// #include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>

// Driver buffer defines
// Allowed RX buffer sizes: 1, 2, 4, 8, 16, 32, 64, 128 or 256
#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE 32
#endif /* TWI_RX_BUFFER_SIZE */

#define TWI_RX_BUFFER_MASK (TWI_RX_BUFFER_SIZE - 1)

#if (TWI_RX_BUFFER_SIZE & TWI_RX_BUFFER_MASK)
#error TWI_RX_BUFFER_SIZE is not a power of 2
#endif

// Allowed TX buffer sizes: 1, 2, 4, 8, 16, 32, 64, 128 or 256
#ifndef TWI_TX_BUFFER_SIZE
#define TWI_TX_BUFFER_SIZE 32
#endif /* TWI_TX_BUFFER_SIZE */

#define TWI_TX_BUFFER_MASK (TWI_TX_BUFFER_SIZE - 1)

#if (TWI_TX_BUFFER_SIZE & TWI_TX_BUFFER_MASK)
#error TWI_TX_BUFFER_SIZE is not a power of 2
#endif

// TWI slave transmitter status codes
#define TWI_STX_ADR_ACK 0xA8             // Own SLA+R has been received; ACK has been returned
#define TWI_STX_ADR_ACK_M_ARB_LOST 0xB0  // Arbitration lost in SLA+R/W as Master; own SLA+R has been received; ACK has been returned
#define TWI_STX_DATA_ACK 0xB8            // Data byte in TWDR has been transmitted; ACK has been received
#define TWI_STX_DATA_NACK 0xC0           // Data byte in TWDR has been transmitted; NOT ACK has been received
#define TWI_STX_DATA_ACK_LAST_BYTE 0xC8  // Last data byte in TWDR has been transmitted (TWEA = “0”); ACK has been received

// TWI slave receiver status codes
#define TWI_SRX_ADR_ACK 0x60             // Own SLA+W has been received ACK has been returned
#define TWI_SRX_ADR_ACK_M_ARB_LOST 0x68  // Arbitration lost in SLA+R/W as Master; own SLA+W has been received; ACK has been returned
#define TWI_SRX_GEN_ACK 0x70             // General call address has been received; ACK has been returned
#define TWI_SRX_GEN_ACK_M_ARB_LOST 0x78  // Arbitration lost in SLA+R/W as Master; General call address has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_ACK 0x80        // Previously addressed with own SLA+W; data has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_NACK 0x88       // Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
#define TWI_SRX_GEN_DATA_ACK 0x90        // Previously addressed with general call; data has been received; ACK has been returned
#define TWI_SRX_GEN_DATA_NACK 0x98       // Previously addressed with general call; data has been received; NOT ACK has been returned
#define TWI_SRX_STOP_RESTART 0xA0        // A STOP condition or repeated START condition has been received while still addressed as Slave

// TWI miscellaneous status codes
#define TWI_NO_STATE 0xF8   // No relevant state information available; TWINT = “0”
#define TWI_BUS_ERROR 0x00  // Bus error due to an illegal START or STOP condition

// Function pointers
void (*p_receive_event)(uint8_t);
//void (*p_enable_slow_ops)(void);

// TWI driver prototypes
void FlushTwiBuffers(void);          // Flush TWI buffers
void TwiDriverInit(uint8_t);         // Initializes TWI hardware and sets the slave device address
void TwiDriverEnable(void);          // Enables TWI slave driver
void TwiTransmitByte(uint8_t);       // Transmits a byte to the TWI master
uint8_t TwiReceiveByte(void);        // Receives a byte from TWI master when it is ready in the receive buffer
bool TwiCheckReceiveBuffer(void);    // Checks for data in the receive buffer. TRUE=Ready to call TwiReceiveByte.
void TwiClearOutputBuffer(void);     // Clears the transmit buffer in case of sync errors
void TwiFillReceiveBuffer(uint8_t);  // Forces a byte into the receive buffer for debugging

#endif /* TWISLAVE_H_ */