The Small Holding Sensor Framework is a collection of arduino code,
hardware designs and server software to manage a sensor collection
in an intelligent manner.

It is specifically designed for small holdings, hobby farms, etc,
but is equally applicable to larger properties, industry and homes.

Currently the range of controllers include:

* Motion Sensing Light
* Brooder Temperature Control
* General Purpose Temperature Control
* Greywater Control
* Solar Hot Water Control
* Temperature Management for Chicken Coops
* Tank Depth Sensing

Each of these was designed for a problem at hand and is in current
use.

Virtually all of the devices use the Arduino Pro Mini or
compatible, and the Digi XBee Series 2 Zigbee module.

Arduino libraries required:

* DHT22
* PciManager
* SoftTimer
* XBee
* DallasTemperatureControl
* OneWire

