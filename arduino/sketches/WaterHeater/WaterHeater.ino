// #include <XBee.h>
#include <DallasTemperature.h>
#include <OneWire.h>
// #include <SoftwareSerial.h>
// #include <Saki.h>

#define ONE_WIRE_IF 9
#define SERIAL_TX 4
#define SERIAL_RX 3
#define RELAY 10
#define RELIEF 11
#define LIGHT_SENSOR 8

float minTemp = 30.0;
float minDiff = 3.0;
float maxTemp = 95.0;
float tank, panel;
int light = -1;
int lowLight = 500;

// SakiManager manager("HW", 3, 1, true);
// SoftwareSerial ser(SERIAL_RX, SERIAL_TX);
OneWire oneWire(ONE_WIRE_IF);
DallasTemperature sensors(&oneWire);

DeviceAddress tankThermometer = { 0x28, 0xbe, 0xb2, 0x97, 0x04, 0x0, 0x0, 0x3f };
DeviceAddress panelThermometer = { 0x28, 0xc7, 0xff, 0x97, 0x04, 0x0, 0x0, 0x34 };

void checkInputs() {
  sensors.requestTemperatures();
  tank = sensors.getTempC(tankThermometer);
  panel = sensors.getTempC(panelThermometer);
//  light = analogRead(LIGHT_SENSOR);
  if (/*light > lowLight && */ panel > minTemp && (panel - tank) > minDiff) {
    digitalWrite(RELAY, LOW);
  } else {
    digitalWrite(RELAY, HIGH);
  }
}

void setup() {
  Serial.begin(9600);
//  ser.begin(9600);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);
//  manager.start(ser);
  sensors.begin();
  delay(5000);
}

void loop() {
//  manager.check();
  checkInputs();
  delay(1000);
}

/*
vim:ft=cpp ai sw=2 expandtab:
*/
