// KY-026 Flame Sensor Test (Arduino Mega 2560 / Pro Mini)
// ........................................................
// 2020-06-15 Gustavo Casanova

#include <Arduino.h>

int led_pin = 13 ;// initializing the pin 13 as the led pin
int flame_sensor_pin = 2 ;// initializing pin 49 (Mega 2560) /2 (Pro-Mini) as the sensor pin

int flame_pin = HIGH ; // state of sensor

void setup (void)  {
  pinMode (led_pin , OUTPUT); // declaring led pin as output pin
  pinMode (flame_sensor_pin , INPUT); // declaring sensor pin as input pin for Arduino
  Serial.begin (9600);// setting baud rate at 9600
}

void loop (void) {
  flame_pin = digitalRead (flame_sensor_pin) ;  // reading from the sensor
  if (flame_pin == HIGH) {
    Serial.println ("FLAME , FLAME , FLAME") ;
    digitalWrite (led_pin, HIGH) ;// if state is high, then turn high the led
  } else {
    Serial.println ("No flame") ;
    digitalWrite (led_pin , LOW) ;  // otherwise turn it low
  }
}