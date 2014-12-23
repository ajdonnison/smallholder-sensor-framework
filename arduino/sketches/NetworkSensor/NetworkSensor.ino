#include <DallasTemperature.h>
#include <OneWire.h>
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

#define zeroFill(x) { int y = x / 10; Serial.write('0'+y); Serial.write('0'+x%10); }
#define hexify(x) { char hex[16] = { '0', '1', '2', '3', '4', '5', '6' ,'7', \
'8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };\
int y = x / 16; \
Serial.write(hex[y%16]); \
Serial.write(hex[x%16]); }

RF24 radio(RADIO_CE,RADIO_CS);
RF24Network network(radio);
AT24C32 eeprom(0);

#define CONFIGURED 0xcf

struct _cfg {
  uint8_t sentinel;
  uint8_t radio_address;
  DeviceAddress temp_sensors[MAX_TEMP_SENSORS];
  float low_point;
  float high_point;
  float reference;
  bool relay;
} cfg;

/*
 * Need to find out how many sensors we have on the 1-wire
 * network and to enumerate them and display the temperature
 * so that the user can identify which is which.
 * Every time we recall the sensors by bus ID it should return
 * the same device each time.
 */
OneWire oneWire(ONE_WIRE_IF);
DallasTemperature tempSensors(&oneWire);

void
configureTemp(void)
{
  float temp;
  int sensor_count;
  Serial.println(F("Checking for temperature sensors"));
  sensor_count = tempSensors.getDeviceCount();
  Serial.print(sensor_count);
  Serial.println(" Sensors found");
  tempSensors.requestTemperatures();
  for (int i = 0; i < MAX_TEMP_SENSORS && i < sensor_count; i++) {
    tempSensors.getAddress(cfg.temp_sensors[i], i);
    temp = tempSensors.getTempC(cfg.temp_sensors[i]);
    Serial.print(i);
    Serial.write(' ');
    Serial.println(temp);
  }
}

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
  configureTemp();
  Serial.println(F("Enter the cut out temperature"));
  while (!Serial.available());
  cfg.low_point = Serial.parseFloat();
  Serial.println(F("Enter the cut in temperature"));
  while (!Serial.available());
  cfg.high_point = Serial.parseFloat();
  Serial.println(F("Enter the reference cut out difference"));
  while (!Serial.available());
  cfg.reference = Serial.parseFloat();
  cfg.sentinel = CONFIGURED;
  eeprom.writeBytes(0, (void *)&cfg, sizeof(cfg));
}

void
printConfig(void) {
  Serial.print(F("Radio Address: "));
  Serial.println(cfg.radio_address, OCT);
  Serial.print(F("Mode: "));
  if (cfg.relay) {
    Serial.println(F("Relay"));
  } else {
    Serial.println(F("Leaf"));
  }
  for (int i = 0; i < MAX_TEMP_SENSORS; i++) {
    Serial.print(F("Temperature "));
    Serial.print(i);
    Serial.print(F(": "));
    for (int j = 0; j< 8; j++) {
      hexify(cfg.temp_sensors[i][j]);
    }
    Serial.println();
  }
  Serial.print(F("Low point: "));
  Serial.println(cfg.low_point);
  Serial.print(F("High point: "));
  Serial.println(cfg.high_point);
  Serial.print(F("Reference difference: "));
  Serial.println(cfg.reference);
}

void
networkScanTask(Task *me)
{
  network.update();
  if (network.available()) {
    RF24NetworkHeader header;
  }
}

void
sensorScanTask(Task *me)
{
  float test;
  float reference;
  tempSensors.requestTemperatures();
  test = tempSensors.getTempC(cfg.temp_sensors[0]);
  reference = tempSensors.getTempC(cfg.temp_sensors[1]);
  Serial.print(test);
  Serial.write(':');
  Serial.println(reference);
 
  if (test < cfg.low_point) {
    Serial.println(F("Test point below low cutout"));
    digitalWrite(RELAY,LOW);
  }
  else if (test > cfg.high_point && test > reference && (test - reference) > cfg.reference) {
    Serial.println(F("High point reached"));
    digitalWrite(RELAY, HIGH);
  } else {
    Serial.println(F("Default condition"));
    digitalWrite(RELAY, LOW);
  }
}


/**
void
cfgScanTask(Task *me)
{
 char buf[32];
 if (!Serial.available()) {
  return;
 }
 memset((void *)buf, 0, 32);
 Serial.readBytes(buf, 31);
 switch (buf[0]) {
   case 'T':
   case 'L':
   case 'H':
   case 'R':
    break;
 }
}

Task cfgScan(1000, cfgScanTask);
*/

Task timeDisplay(10000, printTimeTask);
Task networkScan(10, networkScanTask);
Task sensorScan(5000, sensorScanTask);

void setup(void)
{
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
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
  tempSensors.begin();
  int read = eeprom.readBytes(0, (void *)&cfg, sizeof(cfg));
  if (cfg.sentinel != CONFIGURED) {
    doConfigure();
  }
  printConfig();

  radio.begin();
  network.begin(CHANNEL, cfg.radio_address);
  SoftTimer.add(&networkScan);
  SoftTimer.add(&sensorScan);
  // SoftTimer.add(&cfgScan);
}

// vim:ft=cpp ai sw=2:
