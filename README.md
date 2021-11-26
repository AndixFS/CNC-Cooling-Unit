# CNC-Cooling-Unit
Minimum quantity lubrication

This is a parts list and some schematics for a CNC cooling unit based on the Pico Pi designed by [Andi FS](https://www.facebook.com/profile.php?id=100017118457747 "Andi FS") posted in the [Home Built CNC Router Projects](https://www.facebook.com/groups/HomeBuiltCNCRouterProjects "Home Built CNC Router Projects") group on Facebook. 

Please feel free to commit changes and help improving the design. Just before commercial use reach out to  [Andi FS](https://www.facebook.com/profile.php?id=100017118457747 "Andi FS")

## Features

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

### Partlist for CNC cooling unit

| Amount   | Part (mechanical)  | Link Germany |   Links International    |
|--------- | ------------------ | -------------| ------------------------ |
| 1x       | Aluminium sheet for front panel 195x150x6mm  |   ||  
| 1x       | Aluminium for cooling head, 40x15x15 mm  |   ||
| 1x       | PE 129x109x40mm (for the housing)  |   ||
| 3x       | 12mm button  | https://www.ebay.de/itm/264492799391   | |
| 1x       | Pump  |  https://www.ebay.de/itm/392317740630  | |
| 1x       | arm with central clamping |  https://www.ebay.de/itm/143450105517|  |   
| 4x       | push fittings for cooling head and pipe adapter, which fits your hoses |  |  |
| 12x      | M3x5mm screw (modules)  |  |  |
| 4x       | M2x5mm screw (Pico Pi) |  |  | 
| 4x       |  M3x45mm screw (housing) |  |  | 
| 4x       | M3x12mm screw (LCD) |  |  | 
| 6x       | M3x10mm screw (4x pump, 2x hose adapter) |  |  | 
| 150mm    | silicone hose OD=4mm ID=2mm (pump to adaper) |  |  | | 

| Amount | Part (Electronics) | Link Germany | Links International |
| ------ | ------------------ | ------------ | ------------------- |
| 2x  | 10k Ohm potentiometer  ||
| 1x   | PICO PI  ||
| 1x   | 2 channel photocoupler   | https://www.ebay.de/itm/284102542027?var=585594324600 |
| 1x |2 channel relais with photocouplers   | https://www.ebay.de/itm/353149014364 |
| 1x | Step down power module  | https://www.ebay.de/itm/183980973810 |
| 1x|  Stepper driver carrier board |  https://www.ebay.de/itm/264416015422 |
| 1x  |  Stepper driver A4988  |  https://www.ebay.de/itm/264416015422 |
| 1x |  LCD Module 4x20 | https://www.ebay.de/itm/162451779646 |


### To DO
- Add International Links
- Add Assembly instructions
