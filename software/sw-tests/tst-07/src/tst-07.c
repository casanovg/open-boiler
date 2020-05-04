
//***************************************************************************
//  File Name    : i2cslave.c
//  Version      : 1.0
//  Description  : I2C Slave AVR Microcontroller Interface
//                 MCP23008 Emulation GPIO = PORTD
//  Author       : RWB
//  Target       : AVRJazz Mega168 Learning Board
//  Compiler     : AVR-GCC 4.3.2; avr-libc 1.6.2 (WinAVR 20090313)
//  IDE          : Atmel AVR Studio 4.17
//  Programmer   : AVRJazz Mega168 STK500 v2.0 Bootloader
//               : AVR Visual Studio 4.17, STK500 programmer
//  Last Updated : 12 September 2009
//***************************************************************************
#include <avr/interrupt.h>
#include <avr/io.h>
#include <compat/twi.h>
#include <util/delay.h>
// MCP23008 8 Bit I/O Extention Simulation Address and Register Address
#define MCP23008_ADDR 0x4E
#define IODIR 0x00
#define GPIO 0x09
#define OLAT 0x0A
unsigned char regaddr;   // Store the MCP23008 Requested Register Address
unsigned char regdata;   // Store the MCP23008 Register Address Data
unsigned char olat_reg;  // Simulate MCP23008 OLAT Register
// Simulated OLAT alternative return pattern
#define MAX_PATTERN 7
unsigned char led_pattern[MAX_PATTERN] =
    {0b11110000, 0b01111000, 0b00111110, 0b00011111, 0b00111110, 0b01111000, 0b11110000};
// Simulated OLAT Return Mode: 0-Same as GPIO, 1-Use an alternative LED pattern above
volatile unsigned char olat_mode;

void i2c_slave_action(unsigned char rw_status) {
    static unsigned char iled = 0;
    // rw_status: 0-Read, 1-Write
    switch (regaddr) {
        case IODIR:
            if (rw_status) {
                DDRD = ~regdata;  // Write to IODIR - DDRD
            } else {
                regdata = DDRD;  // Read from IODIR - DDRD
            }

            break;
        case GPIO:
            if (rw_status) {
                PORTD = regdata;  // Write to GPIO - PORTD
                olat_reg = regdata;
            } else {
                regdata = PIND;  // Read from GPIO - PORTD
            }
            break;
        case OLAT:
            if (rw_status == 0) {
                // Read from Simulated OLAT Register
                if (olat_mode) {
                    regdata = led_pattern[iled++];
                    if (iled >= MAX_PATTERN)
                        iled = 0;
                } else {
                    regdata = olat_reg;
                }
            }
    }
}

ISR(TWI_vect) {
    static unsigned char i2c_state;
    unsigned char twi_status;
    // Disable Global Interrupt
    cli();
    // Get TWI Status Register, mask the prescaler bits (TWPS1,TWPS0)
    twi_status = TWSR & 0xF8;

    switch (twi_status) {
        case TW_SR_SLA_ACK:  // 0x60: SLA+W received, ACK returned
            i2c_state = 0;   // Start I2C State for Register Address required

            TWCR |= (1 << TWINT);  // Clear TWINT Flag
            break;
        case TW_SR_DATA_ACK:  // 0x80: data received, ACK returned
            if (i2c_state == 0) {
                regaddr = TWDR;  // Save data to the register address
                i2c_state = 1;
            } else {
                regdata = TWDR;  // Save to the register data
                i2c_state = 2;
            }

            TWCR |= (1 << TWINT);  // Clear TWINT Flag
            break;
        case TW_SR_STOP:  // 0xA0: stop or repeated start condition received while selected
            if (i2c_state == 2) {
                i2c_slave_action(1);  // Call Write I2C Action (rw_status = 1)
                i2c_state = 0;        // Reset I2C State
            }

            TWCR |= (1 << TWINT);  // Clear TWINT Flag
            break;

        case TW_ST_SLA_ACK:   // 0xA8: SLA+R received, ACK returned
        case TW_ST_DATA_ACK:  // 0xB8: data transmitted, ACK received
            if (i2c_state == 1) {
                i2c_slave_action(0);  // Call Read I2C Action (rw_status = 0)
                TWDR = regdata;       // Store data in TWDR register
                i2c_state = 0;        // Reset I2C State
            }

            TWCR |= (1 << TWINT);  // Clear TWINT Flag
            break;
        case TW_ST_DATA_NACK:  // 0xC0: data transmitted, NACK received
        case TW_ST_LAST_DATA:  // 0xC8: last data byte transmitted, ACK received
        case TW_BUS_ERROR:     // 0x00: illegal start or stop condition
        default:
            TWCR |= (1 << TWINT);  // Clear TWINT Flag
            i2c_state = 0;         // Back to the Begining State
    }
    // Enable Global Interrupt
    sei();
}

int main(void) {
    unsigned char press_tm;
    DDRB = 0xFE;  // Set PORTB: PB0=Input, Others as Output
    PORTB = 0x00;
    DDRD = 0xFF;   // Set PORTD to Output
    PORTD = 0x00;  // Set All PORTD to Low
    // Initial I2C Slave
    TWAR = MCP23008_ADDR & 0xFE;  // Set I2C Address, Ignore I2C General Address 0x00
    TWDR = 0x00;                  // Default Initial Value
    // Start Slave Listening: Clear TWINT Flag, Enable ACK, Enable TWI, TWI Interrupt Enable
    TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
    // Enable Global Interrupt
    sei();

    // Initial Variable Used
    olat_reg = 0;
    regaddr = 0;
    regdata = 0;
    olat_mode = 0;
    press_tm = 0;

    for (;;) {
        if (bit_is_clear(PINB, PB0)) {  // if button is pressed
            _delay_us(100);             // Wait for debouching
            if (bit_is_clear(PINB, PB0)) {
                press_tm++;
                if (press_tm > 100)
                    olat_mode ^= 1;  // Toggle the olat_mode
            }
        }
    }
    return 0;
}
/* EOF: i2cslave.c */