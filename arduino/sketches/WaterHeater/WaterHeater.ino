/*
 * Handle circulation pump for a home-made solar hot water system.
 *
 * Uses two Dallas 1-Wire temperature sensors, one at the top of
 * the solar collector, one at the bottom, and only turns the
 * pump on when there is sufficient heat so that it doesn't cause
 * the collector to act as a radiator.
 *
 * TODO:
 * - Automatically determine the addresses of sensors
 * - Re-enabled XBee support for logging.
 */

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
boolean debug = true;

// SakiManager manager("HW", 3, 1, true);
// SoftwareSerial ser(SERIAL_RX, SERIAL_TX);
OneWire oneWire(ONE_WIRE_IF);
DallasTemperature sensors(&oneWire);

DeviceAddress tankThermometer = { 
  0x28, 0xbe, 0xb2, 0x97, 0x04, 0x0, 0x0, 0x3f };
DeviceAddress panelThermometer = { 
  0x28, 0xc7, 0xff, 0x97, 0x04, 0x0, 0x0, 0x34 };

void checkInputs() {
  boolean manual_override = false;

  sensors.requestTemperatures();
  tank = sensors.getTempC(tankThermometer);
  panel = sensors.getTempC(panelThermometer);
  //  light = analogRead(LIGHT_SENSOR);
  if (debug) {
    Serial.print("Tank: ");
    Serial.print(tank);
    Serial.print("  Panel: ");
    Serial.println(panel);
    // Allow manual control, only applies to this time slice.
    if (Serial.available() > 0) {
      int inchar = Serial.read();
      switch (inchar) {
      case '1':
      case 'y':
      case 'Y':
        digitalWrite(RELAY, LOW);
        manual_override = true;
        Serial.println("Turning relay ON");
        break;
      case '0':
      case 'n':
      case 'N':
        digitalWrite(RELAY, HIGH);
        manual_override = true;
        Serial.println("Turning relay OFF");
        break;
      default:
         Serial.print(inchar);
         Serial.println(" Unknown command");  
      }
    }
  }
  if (! manual_override) {
    if (/*light > lowLight && */ panel > minTemp && (panel - tank) > minDiff) {
      digitalWrite(RELAY, LOW);
    } 
    else {
      digitalWrite(RELAY, HIGH);
    }
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
  if (debug) {
    Serial.println("started");
  }
}

void loop() {
  //  manager.check();
  checkInputs();
  delay(2000);
}

/*
vim:ft=cpp ai sw=2 expandtab:
 */

