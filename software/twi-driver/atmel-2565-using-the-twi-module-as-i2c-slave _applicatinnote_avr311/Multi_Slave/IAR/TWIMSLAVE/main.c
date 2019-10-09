/*****************************************************************************
*
* Atmel Corporation
*
* File              : main.c
* Compiler          : IAR C/C++ Compiler for AVR
                      6.30.1 (6.30.1.687)

 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
*
* Supported devices : All devices with a TWI module the has a slave address mask
*                     register. The example is written for the ATmega48.
*
* AppNote           : AVR311 - TWI Slave Implementation
*
* Description       : Example of how to use the driver for TWI slave 
*                     communication. This version utilises the address mask register
*                     to enable response on several TWI slave addresses.
*
****************************************************************************/


#include <ioavr.h>
#include <inavr.h>
#include "TWI_Slave.h"

// Sample TWI transmission commands
#define TWI_CMD_MASTER_WRITE 0x10
#define TWI_CMD_MASTER_READ  0x20

// The AVR can be waken up by a TWI address match from all sleep modes,
// But it only wakes up from other TWI interrupts when in idle mode.
// If POWER_MANAGEMENT_ENABLED is defined the device will enter power-down 
// mode when waiting for a new command and enter idle mode when waiting
// for TWI receives and transmits to finish.
#define POWER_MANAGEMENT_ENABLED

// When there has been an error, this function is run and takes care of it
unsigned char TWI_Act_On_Failure_In_Last_Transmission ( unsigned char TWIerrorMsg );


void main( void )
{
  unsigned char messageBuf[TWI_BUFFER_SIZE];
  unsigned char TWI_slaveAddress, TWI_slaveAddress2, TWI_slaveAddressMask;
  
  // LED feedback port - connect port B to the STK500 LEDS
  DDRB  = 0xFF; // Set to ouput
  PORTB = 0x55; // Startup pattern

  // Own TWI slave address
  TWI_slaveAddress     = (0x10<<TWI_ADR_BITS);
  TWI_slaveAddress2    = (0x11<<TWI_ADR_BITS);                  // Alternativ slave address to respond to.
  TWI_slaveAddressMask = TWI_slaveAddress ^ TWI_slaveAddress2;  // XOR the addresses to get the address mask.

  // Initialise TWI module for slave operation. Include address and/or enable General Call.
  TWI_Slave_Initialise( (unsigned char)(TWI_slaveAddress | (TRUE<<TWI_GEN_BIT)), TWI_slaveAddressMask); 
                       
  __enable_interrupt();

  // Start the TWI transceiver to enable reseption of the first command from the TWI Master.
  TWI_Start_Transceiver();

  // This example is made to work together with the AVR315 TWI Master application note. In adition to connecting the TWI
  // pins, also connect PORTB to the LEDS. The code reads a message as a TWI slave and acts according to if it is a 
  // general call, or an address call. If it is an address call, then the first byte is considered a command byte and
  // it then responds differently according to the commands.

  // This loop runs forever. If the TWI is busy the execution will just continue doing other operations.
  for(;;)
  { 
    #ifdef POWER_MANAGEMENT_ENABLED
      // Sleep while waiting for TWI transceiver to complete or waiting for new commands.
      // If we have data in the buffer, we can't enter sleep because we have to take care
      // of it first.
      // If the transceiver is busy, we enter idle mode because it will wake up by all TWI
      // interrupts.
      // If the transceiver not is busy, we can enter power-down mode because next receive
      // should be a TWI address match and it wakes the device up from all sleep modes.
      if( ! TWI_statusReg.RxDataInBuf ) {
        if(TWI_Transceiver_Busy()) {
          MCUCR = (1<<SE)|(0<<SM2)|(0<<SM1)|(0<<SM0); // Enable sleep with idle mode
        } else {
          MCUCR = (1<<SE)|(0<<SM2)|(1<<SM1)|(0<<SM0); // Enable sleep with power-down mode
        }
        __sleep();
      } else {
        __no_operation(); // There is data in the buffer, code below takes care of it.
      }
    #else // No power management
      // Here you can add your own code that should be run while waiting for the TWI to finish    
      __no_operation(); // Put own code here.
    #endif
      
    
    // Check if the TWI Transceiver has completed an operation.
    if ( ! TWI_Transceiver_Busy() )                              
    {
      // Check if the last operation was successful
      if ( TWI_statusReg.lastTransOK )
      {
        // Check if the last operation was a reception
        if ( TWI_statusReg.RxDataInBuf )
        {
          TWI_Get_Data_From_Transceiver(messageBuf, 2);         
          // Check if the last operation was a reception as General Call        
          if ( TWI_statusReg.genAddressCall )
          {
            // Put data received out to PORTB as an example.        
            PORTB = messageBuf[0];
          }               
          else // Ends up here if the last operation was a reception as Slave Address Match   
          {
            // Example of how to interpret a command and respond.
            
            // TWI_CMD_MASTER_WRITE stores the data to PORTB
            if (messageBuf[0] == TWI_CMD_MASTER_WRITE)
            {
              PORTB = messageBuf[1];                            
            }
            // TWI_CMD_MASTER_READ prepares the data from PINB in the transceiver buffer for the TWI master to fetch.
            if (messageBuf[0] == TWI_CMD_MASTER_READ)
            {
              messageBuf[0] = PINB;
              TWI_Start_Transceiver_With_Data( messageBuf, 1 );
            }
          }
        }                
        else // Ends up here if the last operation was a transmission  
        {
            __no_operation(); // Put own code here.
        }
        // Check if the TWI Transceiver has already been started.
        // If not then restart it to prepare it for new receptions.             
        if ( ! TWI_Transceiver_Busy() )
        {
          TWI_Start_Transceiver();
        }
      }
      else // Ends up here if the last operation completed unsuccessfully
      {
        TWI_Act_On_Failure_In_Last_Transmission( TWI_Get_State_Info() );
      }
    }
  }
}


unsigned char TWI_Act_On_Failure_In_Last_Transmission ( unsigned char TWIerrorMsg )
{
                    // A failure has occurred, use TWIerrorMsg to determine the nature of the failure
                    // and take appropriate actions.
                    // Se header file for a list of possible failures messages.
  
                    // This very simple example puts the error code on PORTB and restarts the transceiver with
                    // all the same data in the transmission buffers.
  PORTB = TWIerrorMsg;
  TWI_Start_Transceiver();
                    
  return TWIerrorMsg; 
}

/*  
  // A simplified example.
  // This will store data received on PORTB, and increment it before sending it back.

  TWI_Start_Transceiver( );    
         
  for(;;)
  {
    if ( ! TWI_Transceiver_Busy() )                              
    {
      if ( TWI_statusReg.RxDataInBuf )
      {
        TWI_Get_Data_From_Transceiver(&temp, 1);  
        PORTB = temp;
      }
      temp = PORTB + 1;
      TWI_Start_Transceiver_With_Data(&temp, 1); 
    }
    __no_operation();   // Do something else while waiting
  }
}

*/
