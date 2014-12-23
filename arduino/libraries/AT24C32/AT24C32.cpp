#include "Arduino.h"
#include "AT24C32.h"
#include <Wire.h>

AT24C32::AT24C32(int device)
{
  device_address = 0x50 + (device & 0x07);
}

void
AT24C32::setAddress(uint16_t address)
{
  Wire.beginTransmission(device_address);
  Wire.write(address >> 8);
  Wire.write(address & 0xff);
}

uint8_t
AT24C32::readNextByte(void)
{
  uint16_t data = WIRE_ERROR;
  uint8_t rdata;
  Wire.requestFrom(device_address, (uint8_t)1);
  if (Wire.available()) {
    rdata = Wire.read();
    data = rdata;
  }
  return data;
}

uint8_t
AT24C32::readByte(uint16_t address)
{
  setAddress(address);
  Wire.endTransmission();
  return readNextByte();
}

uint8_t
AT24C32::readBytes(uint16_t address, void * data, uint8_t count)
{
  uint16_t read = 0;
  for (int i = 0; i < count; i+=32) {
    uint8_t rcount = 32;
    if (count < (i+32)) {
      rcount = count - i;
    }
    setAddress(address);
    Wire.endTransmission();
    Wire.requestFrom(device_address, rcount);
    uint8_t * ptr = (uint8_t *)data + i;
    while (Wire.available()) {
      read++;
      *ptr++ = Wire.read();
      address++;
    }
  }
  return read;
}

uint8_t
AT24C32::writeByte(uint16_t address, uint8_t data)
{
  uint8_t count;
  setAddress(address);
  count = Wire.write(data);
  Wire.endTransmission();
  return count;
}

uint16_t
AT24C32::writeBytes(uint16_t address, void * data, uint8_t count)
{
  uint16_t written;
  for (int i = 0; i < count; i+= 32) {
    int rcount = 32;
    if (count < (i+32)) {
      rcount = count - i;
    }
    setAddress(address+i);
    uint8_t * ptr = (uint8_t *)data + i;
    written += Wire.write((const uint8_t *)ptr, rcount);
    Wire.endTransmission();
  }
  return written;
}



