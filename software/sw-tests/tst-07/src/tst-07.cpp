//
// Wire Slave Receiver
//

//#include <Arduino.h>
#include <tst-07.h>

// Setup
void setup() {
    // put your setup code here, to run once:
    Wire.begin(12);                // join i2c bus with address #4
    Wire.onReceive(receiveEvent);  // register event
    Serial.begin(9600);            // start serial for output
}

// Main loop
void loop() {
    // put your main code here, to run repeatedly:
    delay(100);
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {
    while (1 < Wire.available())  // loop through all but the last
    {
        char c = Wire.read();  // receive byte as a character
        Serial.print(c);       // print the character
    }
    int x = Wire.read();  // receive byte as an integer
    Serial.println(x);    // print the integer
}
