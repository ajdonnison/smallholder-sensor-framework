# Greywater Control System

The [Wattworks](http://www.wattworks.com.au) system circa 1995
consisted of a 200l low-profile square tank with
three floats, a sump pump, and a number of solenoids for directing the
water to various locations.  Additionally it had a cistern level sensor
for converting your toilet to run on greywater.

This system came with a basic controller that met the immediate needs
but did have a few drawbacks.

Originally this was replaced by a system based on an PIC16F84 MCU, later
replaced with an 8051 based system, which
had a serial connection to a host for control/display/logging.

Since then the system is no longer used to manage toilet filling, and
was simplified to just push the stored grewater out to the orchard. At
this time the MCU-based controller was replaced with a simple latching
relay system.

As part of the testing of sensor/controller/radio systems I decided to
rework this controller as an Arduino-based system using an XBee for
communications.

In the future this controller will be enhanced to be able to handle
the full range of functions the 8051 controller did.

Note: It would appear that the Wattworks site has not been updated
for several years and none of the listed distributors list any
Wattworks products.  This controller could easily work with a
simple tank/pump/solenoid system.
