/*
    Roca Victoria 20/20T Boiler AC Motor Control
 */

#include "victoria-20-20t-ext-control.h"

void setup() {
  // put your setup code here, to run once:
  DDRB = (1 << PB1);
}

void loop() {
  // put your main code here, to run repeatedly:
  PORTB |= (1 << PB1);
  _delay_ms(250);
  PORTB &= ~(1 << PB1);
  _delay_ms(250);
}