# Programmable Temperature Controller

Originally developed in a hurry with parts out of my parts bin, this
was designed to manage a heater to keep water at the correct temperature
to scald poultry to make plucking easier.

Uses a DS18B20 1-Wire temperature sensor and an Arduino Pro Mini

This was connected to a 20 litre plastic pail with a 2kW heating element.

Since then it has been extensively revamped to use a display and two
button setup functionality, along with the ability to set either Heat
or Cool mode.

Heat Mode: In heat mode the relay is activated when the temperature falls
below the set point, minus the hysterisis.  It turns it off again when the
temperature hits the set point.

Cool Mode: In cool mode the relay is activated when the temperature rises
to the set point. It turns off again when the temperature falls below the set
point minus the hysterisis.

Configuration:

Pressing either the UP or DOWN button for 2 seconds puts the system into
setup mode.  In setup mode there are a number of options, selectable by pressing
either up or down button to cycle through then holding either button down for 2 seconds.

HC = Heat/Cool mode select
TC = Temperature (Degrees C) Set Point setting
Diff = Temperature Hysterisis (Difference) in Degrees C
Set = Save current settings

When Set is displayed, holding either button down for 2 seconds causes the current
settings to be saved, and setup mode to exit.

Note:  There is a light switch (seconday relay) function that has been added
for local use - it is expected to form the basis of further developments
into controlling either the primary or secondary relay via a timer.
