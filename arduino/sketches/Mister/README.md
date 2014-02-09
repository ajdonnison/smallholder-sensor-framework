# Poultry comfort misting system

Handles up to four zones, each with a temperature sensor and a
solenoid valve controlling a misting spray.  When the temperature
reaches a set point (we currently use 36C) it will enabled the
solenoid valve and spray a fine mist of water to cool the air
in the poultry yards.

In order to save water and provide for the slow response of the
temperature sensor, the water is turned on for 30 seconds with a
4 minute delay between each spray.  All of this is programmable.

Temperature sensors are DS18B20

The solenoid valves are 12VDC, although would work with standard
24VAC solenoids provided a suitable power supply was used.

To Do:

- Automatically detect temperature sensors and associate with zones
- Remote data logging/control

