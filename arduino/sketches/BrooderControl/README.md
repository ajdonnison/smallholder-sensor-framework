# Poultry Brooder Temperature Control

This was built in a hurry.  We had chickens and ducks hatching right,
left, and centre, and our two brooders were already in use.  We bought
a plastic box of suitable size and then I ratted through my component
store to pull together a temperature controller for it.

This uses two common-anode 7-segment LEDs that were left over originally from
a digital clock project that I'd made some 20 years ago, along with
a BCD to 7-segment decoder driver I had lying around.  The switches
came from a Fisher & Paykel washing machine that I'd pulled the motor
out of for another project, as did the indicator LEDs.
The temperature sensor was a DHT22 and the controller an Arduino Pro Mini.

Future Options:

- Add a timer and automatically scale the temperature back
- Add the ability to manage multiple brooders with the one display
- Add wireless comms to work with other components
