/*
 * Handle circulation pump for a home-made solar hot water system.
 *
 * Uses two Dallas 1-Wire temperature sensors, one at the top of
 * the solar collector, one at the bottom, and only turns the
 * pump on when there is sufficient heat so that it doesn't cause
 * the collector to act as a radiator.
 *
 * TODO:
 * - Re-enabled XBee support for logging.
 */

#include <DallasTemperature.h>
#include <OneWire.h>
#include <PciManager.h>
#include <SoftTimer.h>
#include <Debouncer.h>
#include <BlinkTask.h>
#include <avr/eeprom.h>

#define ONE_WIRE_IF 12
#define SCAN 9
#define RELAY 10
#define INDICATOR 13

float minTemp = 30.0;
float minDiff = 3.0;
float maxTemp = 95.0;
float tank, panel;
boolean debug = true;
boolean inError = false;

OneWire oneWire(ONE_WIRE_IF);
DallasTemperature sensors(&oneWire);

DeviceAddress tankThermometer = { 
  0x28, 0xbe, 0xb2, 0x97, 0x04, 0x0, 0x0, 0x3f };
DeviceAddress panelThermometer = { 
  0x28, 0xc7, 0xff, 0x97, 0x04, 0x0, 0x0, 0x34 };

BlinkTask indicateError(INDICATOR, 500);
BlinkTask indicateScan(INDICATOR, 150);

void writeConfig() {
  uint32_t config_set = 0xdeadbeef;
  eeprom_busy_wait();
  eeprom_write_block(tankThermometer, (void *)8, 8);
  eeprom_busy_wait();
  eeprom_write_block(panelThermometer, (void *)16, 8);
  eeprom_busy_wait();
  eeprom_write_dword((uint32_t *)0, config_set);
}

/**
 * Scan for sensors.  This assumes that the panel sensor
 * will be at a higher temperature than the tank sensor.
 * It can be used for testing or if the sensors need to be
 * replaced.
 */
void scanForSensors() {
  DeviceAddress addr1, addr2;
  float temp1, temp2;
  boolean scanComplete = false;
  int scanCount = 100;
  int devCount = 0;

  inError = true; // Disables normal read.
  indicateError.stop(); // Just in case
  indicateScan.start();
  if (debug) {
    Serial.println("Starting scan");
  }
  if ((devCount = sensors.getDeviceCount()) < 2) {
    if (debug) {
      Serial.print("Found ");
      Serial.print(devCount);
      Serial.println(" devices");
    }
    indicateScan.stop();
    indicateError.start();
    return;
  }
  sensors.getAddress(addr1, 0);
  sensors.getAddress(addr2, 1);
  if (! sensors.validAddress(addr1) || ! sensors.validAddress(addr2)) {
    if (debug) {
      Serial.println("No valid addresses on bus");
    }
    indicateScan.stop();
    indicateError.start();
    return;
  }
  while (!scanComplete && --scanCount >= 0) {
    // Grab the temp, if one is higher than the other, it is the panel.
    sensors.requestTemperatures();
    temp1 = sensors.getTempC(addr1);
    temp2 = sensors.getTempC(addr2);
    if (addr1 != addr2) {
      if (addr1 > addr2) {
        sensors.getAddress(panelThermometer, 0);
        sensors.getAddress(tankThermometer, 1);
      } else {
        sensors.getAddress(tankThermometer, 0);
        sensors.getAddress(panelThermometer, 1);
      }
      if (debug) {
        Serial.println("Found sensors");
      }
      writeConfig();
      scanComplete = true;
      inError = false;
    }
  }
  indicateScan.stop();
  if (! scanComplete) {
    indicateError.start();
    inError = true;
  }
}

void checkInputs(Task *me) {
  if (inError) {
    return;
  }
  sensors.requestTemperatures();
  tank = sensors.getTempC(tankThermometer);
  panel = sensors.getTempC(panelThermometer);
  if (debug) {
    Serial.print("Tank: ");
    Serial.print(tank);
    Serial.print("  Panel: ");
    Serial.println(panel);
  }
  if (panel > minTemp && (panel - tank) > minDiff) {
    digitalWrite(RELAY, HIGH);
    digitalWrite(INDICATOR, HIGH);
  } 
  else {
    digitalWrite(RELAY, LOW);
    digitalWrite(INDICATOR, LOW);
  }
}

Debouncer scanButton(SCAN, MODE_CLOSE_ON_PUSH, scanForSensors, NULL);
Task checkTempTask(1000, checkInputs);

/**
 * Grab the configured list of sensors, if known,
 * and check if they are on the 1-wire interface.
 * If not we indicate an error.
 */
void checkSensors() {
  uint32_t config_set;
  indicateScan.start();
  eeprom_busy_wait();
  config_set = eeprom_read_dword((const uint32_t *)0);
  if (config_set == 0xdeadbeef) {
    eeprom_busy_wait();
    eeprom_read_block(tankThermometer, (const void *)8, 8);
    eeprom_busy_wait();
    eeprom_read_block(panelThermometer, (const void *)16, 8);
  }
  // If we didn't find anything in the config we assume the
  // default parameters.  Either way we now check the bus.
  if (! sensors.isConnected(tankThermometer) || ! sensors.isConnected(panelThermometer)) {
    indicateScan.stop();
    indicateError.start();
    inError = true;
  } else {
    indicateScan.stop();
    inError = false;
  }
}

void setup() {
  inError = false;
  Serial.begin(9600);
  pinMode(RELAY, OUTPUT);
  pinMode(SCAN, INPUT);

  digitalWrite(RELAY, LOW);
  digitalWrite(SCAN, HIGH);

  sensors.begin();

  checkSensors();

  PciManager.registerListener(SCAN, &scanButton);
  SoftTimer.add(&checkTempTask);

  if (debug) {
    Serial.println("started");
  }
}

/*
vim:ft=cpp ai sw=2 expandtab:
 */
