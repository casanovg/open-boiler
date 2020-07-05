# ![Open-Boiler](https://github.com/casanovg/open-boiler/blob/media/open-boiler-logo.png)
This project aims to replace a Roca Victoria 20/20 domestic combined boiler control PCB with an open-hardware/software alternative. It is possible that in the future its application will also be extended to other boiler models and manufacturers.

## The story behind the project
I bought this “Roca Victoria 20/20 T” combined boiler in 2002 and it worked perfectly for approximately 5 years, then the first problems with the control sensors appeared. First was the ionization probe, which detects when the burner is on. Over time the exhaust gas flowmeter also started to fail. It is interesting to mention that the failure of the first sensor leaves the boiler with an error indication waiting for a reset, so if this happens to you in the middle of a cold winter night when you sleep, in the morning you get up at the trans-Siberian express.

The other sensor, which checks if the combustion fume extractor generates pressure in the outlet pipe, has a bad habit of failing just when you are taking a nice warm bath, so that the cold water punch you receive when the boiler suddenly turns off with error makes you change the note that you were singing in the shower for a desperate scream worthy of a B-class horror movie. That's how I kept replacing the sensors, which are not cheap, regularly every 2 or three years.


After searching in various forums and websites, I concluded in principle that this boiler model was a nightmare and that I had to spend a significant sum of money to replace it and achieve peace of mind. But then I was sure that nothing guarantees that a new boiler does not start with the same problems in about 5 years since, searching a little, I found that these equipment's sensors seem to be a standard that, are more or less, common to all boiler manufacturers. So I kept “patching” the sensors' problems as they arose until, one day in July 2019, the control board said “enough” and needed to be replaced, or buy a new boiler. The PCB, without sensors, costs nothing less than about USD 330 where I live, and a new boiler about USD 1.500.

Before throwing the boiler away, I decided to take it apart once more to see what I could do. I found that,  beyond the dislike of a boiler that had left me without DHW and central heating, I could appreciate in detail that the mechanical built was, indeed, quite well made. Noble materials, stainless steel, copper, good finishes. No water leakage, and not even a spot of rust in 17 years. So, tired of the electronics and sensors of this boiler, I decided to try to replace the control PCB with one made by myself, and therefore being able to take control of the installed components and, above all, the boiler firmware, to be able to decide when and how this gadget must stop with an unrecoverable error, or inform and move on when it is not something so serious. The idea is to be able to ensure that when a problem is detected, it is a problem (lack of gas, the flame extinguished by the wind, etc.) and not a sensor failure that detects it.

## Opening the black box
The first thing to consider is to start seeing the boiler, not as a black box any longer (or rather white in this case), but as a set of inputs and outputs to which you have to apply logic to make it work orchestrated. This particular boiler model is composed as follows:

## Inputs:
1. DHW temperature sensor: Honeywell T7335D NTC thermistor  (10 KΩ at 25 °C).
2. Central heating temperature sensor: : Honeywell T7335D NTC thermistor  (10 KΩ at 25 °C).
3. DHW temperature setting: Piher 10 KΩ through-hole PCB potentiometer.
4. DC temperature setting: Piher 10 KΩ through-hole PCB potentiometer.
5. System mode setting: Piher 10 KΩ through-hole PCB potentiometer.
6. Flame sensor: ionization probe -> to be replaced by a KY-026 infrared light sensor.
7. Flue exhaust flow sensor: Pressure switch 12-15 mm ca -> initially preserved but with an overridding option by software. Maybe it will be replaced by a Bosch BMP280 barometric sensor in the future.
8. Overheating sensor: Campini Ty60R 105 °C manual reset-thermostat.

## Outputs:
1. Safety gas valve: Solenoid 12 VDC, 31 Ω.
2. 7.000 Kcal/h gas valve: Solenoid 12 VDC, 31 Ω.
3. 12.000 Kcal/h gas valve: Solenoid 12 VDC, 31 Ω.
4. 20.000 Kcal/h gas valve: Solenoid 12 VDC, 31 Ω.
5. Flue gas extractor: 220 VAC electric fan.
6. Heating water pump: 3-speed 220 VAC electric pump.
6. Electronic spark igniter: 18,000 V output, 12 and 5 VDC inputs.
