#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include <Time.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <PciManager.h>
#include <SoftTimer.h>
#include <AT24C32.h>
#include "setup.h"

#define zeroFill(x) { int y = x / 10; Serial.print(y); Serial.print(x%10); }

RF24 radio(RADIO_CE,RADIO_CS);
RF24Network network(radio);
AT24C32 eeprom(0);

#define CONFIGURED 0xab

struct _cfg {
  uint8_t sentinel;
  uint8_t radio_address;
  bool relay;
} cfg;

void
configureTime(void)
{
  unsigned long set = 0L;
  const unsigned long DEFAULT_TIME = 1400000000;

  Serial.println(F("Time is not set"));
  while (set < DEFAULT_TIME) {
    Serial.println(F("Please enter time as seconds in epoch"));
    while (!Serial.available()) ;
    set = Serial.parseInt();
  }
  RTC.set(set);
  setTime(set);
}

void
printTime()
{
  Serial.print(year());
  Serial.print("-");
  zeroFill(month());
  Serial.print("-");
  zeroFill(day());
  Serial.print(" ");
  zeroFill(hour());
  Serial.print(":");
  zeroFill(minute());
  Serial.print(":");
  zeroFill(second());
  Serial.println();
}

void
printTimeTask(Task *me) {
  printTime();
}

void
doConfigure(void) {
  int val;

  cfg.radio_address = 0;
  while ( ! cfg.radio_address) {
    Serial.println(F("Enter the octal address for the radio"));
    while (!Serial.available()) ;
    cfg.radio_address = Serial.parseInt();
  }
  Serial.println();
  Serial.println(F("Is the radio a relay? (y/n)"));
  while (!Serial.available()) ;
  val = Serial.read();
  cfg.relay = (val == 'y' || val == 'Y');
  cfg.sentinel = CONFIGURED;
  eeprom.writeBytes(0, (void *)&cfg, sizeof(cfg));
}

void
printConfig(void) {
  Serial.print(F("Radio Address: "));
  Serial.println(cfg.radio_address);
  Serial.print(F("Mode: "));
  if (cfg.relay) {
    Serial.println(F("Relay"));
  } else {
    Serial.println(F("Leaf"));
  }
}

void
networkScanTask(Task *me)
{
  network.update();
  if (network.available()) {
    RF24NetworkHeader header;
  }
}

Task timeDisplay(10000, printTimeTask);
Task networkScan(10, networkScanTask);

void setup(void)
{
  Serial.begin(9600);
  Serial.println(F("Starting"));
  // First, check that we have time
  setSyncProvider(RTC.get);
  timeStatus();
  while (! RTC.chipPresent()) {
    Serial.println(F("Failed to find RTC ... Check connections"));
    delay(4000);
  }

  if (timeStatus() != timeSet) {
    configureTime();
  } else {
    Serial.println(F("Time already set"));
  }
  SoftTimer.add(&timeDisplay);
  SPI.begin();

  // check our configuration
  int read = eeprom.readBytes(0, (void *)&cfg, sizeof(cfg));
  if (cfg.sentinel != CONFIGURED) {
    doConfigure();
  }
  printConfig();

  radio.begin();
  network.begin(CHANNEL, cfg.radio_address);
  SoftTimer.add(&networkScan);
}

// vim:ft=cpp ai sw=2:
