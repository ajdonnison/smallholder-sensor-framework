Generic Network Sensor
=====================

While this is designed to be a generic network sensor, there are currently
some constraints in that it relies on there being a number of temperature
sensors (limited at the present to two), and a small number of outputs
(currently two relays and an indicator).

The code can be configured to use a 4 digit 7-segment LED display which
also implies a two-button configuration mechanism.  This is optional and
can be disabled in setup.h

Also optional is an EEPROM for configuration storage, and an RTC chip for
keeping time.

Basic Functionality
-------------------

* Can act as heating or cooling controller
* Single temperature sensor and hysteresis adjustment OR
* Dual temperature sensors with set points and minimum difference
* Separate timer controlled output
* Fully configurable via two-button interface
* Fully configurable over the air

Work needed
-----------

* Separate out the temperature management
* Add support for non-temperature sensors
* Optimise the code (currently close to size limits)
* Use internal chip EEPROM when I2C chip missing
