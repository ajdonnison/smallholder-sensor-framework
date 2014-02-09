"""
    First cut of an XBee/ZigBee controller.
    Proposed Logic Flow:
    - Send Node Discovery command
    - On receipt build up list of nodes, their addresses and types
    - Node identifier strings indicate capabilities, according to
      the following table:
      bytes 0-2 = Fixed string indicating product class, 'SET' is
        generic automation, 'SGF' is agricultural
      byte 3 = 'A' - Arduino attached that supports ID request
               'W' - Weather station 
               'G' - Analog threshold device
               'D' - Digital threshold device
               'M' - Mixed device
               'S' - Generic Sensor
               'C' - Generic Actuator
      bytes 4-11 = Unique Identifier (HEX) used in config files
    - Based on NI, either:
      - Send an ID request to arduino connected devices OR
      - Use remote AT commands to identify characteristics
    - Read the config table to determine any programattic requirements
      e.g. Send command at specific time each day
           Alert when x received
  
"""
from sgf import controller
import serial
import time

ser = serial.Serial('/dev/ttyUSB0', 9600)
xb = controller.ZigBee(ser)
while True:
    try:
        time.sleep(0.001)
    except KeyboardInterrupt:
        break

xb.stop()
ser.close()
# vim: ai sw=4 expandtab:
