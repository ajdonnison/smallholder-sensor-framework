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
compatible. Originaly using the Digi XBee, now mostly settled on
the NRF24L+ low-cost radio modules.

Most work recently is on creating a generic sensor that can be adapted
to most situations.  This has been based on the General Purpose Temperature
Controller, and in its first incarnation is used to manage either one or
two temperature sensors for either heating or cooling.  This has now
supplanted the Brooder Temperature Control, General Purpose Temperature
Control, Solar Hot Water Control and is being usd in all of these situations
as well as managing a Solar ceiling cooler.  The idea is to get this down
to a few small boards that can be replicated easily for differing purposes.

Arduino libraries required:

* DHT22
* PciManager
* SoftTimer
* DallasTemperatureControl
* OneWire

