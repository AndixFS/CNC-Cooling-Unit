# CNC-Cooling-Unit
Minimum quantity lubrication

This small programm was assembled with a pico pi example for i2c LCD.
Following functions are added
- PWM for a stepper with 50/50 duty cycle and variable frequency 
- ADC for potentiometers
- GPIO for buttons (recognition of pushed, released and hold)
- actual control program

The unit have following functions:
- Pump with the given speed (via poti)
- Boost to fill the hose, boost time via poti
- control 2 valves: air and lubricant
- cooling only with air

I also include plans for hardware (DXF) and a part list.


