# Poultry comfort misting system

Handles up to four zones, each with a temperature sensor and a
solenoid valve controlling a misting spray.  When the temperature
reaches a set point (we currently use 36C) it will enable the
solenoid valve and spray a fine mist of water to cool the air
in the poultry yards.

In order to save water and provide for the slow response of the
temperature sensor, the water is turned on for 30 seconds with a
4 minute delay between each spray.  All of this is programmable.

With more efficient sprays such as the newer misting systems available
in gardening outlets, a more realistic 5 minutes of run time can be
used.

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
* m - Configure maximum (cut-in) temperature
* n - Configure minimum (cut-out) temperature
* w - Configure minimum wait time between runs
* r - Configure maximum run time

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

### Setting Configurable Values

In the main menu if you enter one of the configure options (`m`, `n`, `w` or `r`) it will wait for up to 5 seconds for a float value to be entered, which will then be written to EEPROM.

Settable values are:

| option | range | default | usage |
| ---- | ---- | ---- | ---- |
| m | 0-50 | 36 | Degrees C at which pump will be turned on |
| n | 0-50 | Max less 2C | Degrees C at which pump will be turned off |
| w | 0-9999 | 240 | Seconds to wait after pump operation stops before allowing it to trigger again |
| r | 0-9999| 30 | Seconds of pumping time before it stops for the wait period |


