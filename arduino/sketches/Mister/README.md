# Poultry comfort misting system

Handles up to four zones, each with a temperature sensor and a
solenoid valve controlling a misting spray.  When the temperature
reaches a set point (we currently use 36C) it will enable the
solenoid valve and spray a fine mist of water to cool the air
in the poultry yards.

In order to save water and provide for the slow response of the
temperature sensor, the water is turned on for 30 seconds with a
4 minute delay between each spray.  All of this is programmable.

Temperature sensors are DS18B20

The solenoid valves are 12VDC, although would work with standard
24VAC solenoids provided a suitable power supply was used.

To Do:

- Remote data logging/control

## Menu mode

Connecting a serial port at 9600/8/1/N will allow for monitoring
and control.

### Main Menu

* d - Toggles DEBUG mode
* t - Turns on the TEST mode menu
* s - Shows current status
* a - Turns on Association mode menu
* m - Configure maximum temperature

### Test Menu

* x - Exits to main menu
* s - Show current status
* 1-4 - Toggle on/off status of associated solenoid

### Association Menu

* x - Writes config and exits to main menu
* s - Scan 1-wire bus for temperature sensors
* n - Select next sensor
* p - Select previous sensor
* 1-4 - Associate current sensor with solenoid

### Debug Mode

In debug mode each tick will show the current temperature of each sensor.  When a solenoid is turned on or off that event will also be shown.

### Setting max temp

When `m` is chosen in the main menu the system waits for up to 5 seconds for a valid float to be entered.  This is then saved as the maximum temperature.

