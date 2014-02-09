/*
 * Misting control for chicken coops.
 *
 * Being in a very hot and dry area, we were at the risk of losing
 * birds to heat stress.  Most of the yards have shadecloth attached
 * so this was designed to mist water over them periodically once
 * the trigger temperature is reached to create an evaporative
 * cooling system.
 *
 * So far this has been working brilliantly.
 *
 */
#include <DallasTemperature.h>
#include <OneWire.h>
#include <DelayRun.h>
#include <SoftTimer.h>

#define ONE_WIRE_IF 2
#define PUMP_OFFSET 3
#define DEVICE_COUNT 2
#define MAX_DEVICE_COUNT 4
#define MAX_TEMP 36.0
#define MIN_WAIT 240
#define ON_TIME 30

OneWire dataBus(ONE_WIRE_IF);
DallasTemperature devManager(&dataBus);
boolean startup_delay = true;

typedef struct st_devlist {
  DeviceAddress * sensor;
  int pump;
  int status;
  float temp;
  int time;
} DEVICE_LIST;


DEVICE_LIST nodes[DEVICE_COUNT];

DeviceAddress  sensor_list[] = {
  { 0x28, 0x22, 0x5d, 0x97, 0x04, 0x00, 0x00, 0x46 },
  { 0x28, 0x79, 0x47, 0x97, 0x04, 0x00, 0x00, 0x51 }
};

boolean stopPump(Task *me) {
  for (int i = 0; i < DEVICE_COUNT; i++) {
    if (nodes[i].status == 1) {
      Serial.print("Pump ");
      Serial.print(nodes[i].pump);
      Serial.println(" Off");
      digitalWrite(nodes[i].pump, LOW);
      nodes[i].status = 2;
      nodes[i].time = MIN_WAIT;
    }
  }
  return false;
}

DelayRun stopPumpTask(ON_TIME * 1000, stopPump);

void checkTemp(Task *me) {
  devManager.requestTemperatures();
  if (startup_delay) {
    return;
  }
  for (int i = 0; i < DEVICE_COUNT; i++) {
    nodes[i].temp = devManager.getTempC(*(nodes[i].sensor));
    Serial.print("Temp ");
    Serial.print(nodes[i].pump);
    Serial.print(": ");
    Serial.println(nodes[i].temp);
    switch (nodes[i].status) {
      case 2:
        if (nodes[i].time-- < 0) {
          nodes[i].status = 0;
        }
        break;
      case 0:
        if (nodes[i].temp > MAX_TEMP) {
          Serial.print("Turning on pump ");
          Serial.println(nodes[i].pump);
          nodes[i].status = 1;
          digitalWrite(nodes[i].pump, HIGH);
          stopPumpTask.startDelayed();
        }
        break;
    }
  }
}

boolean clearDelay(Task *me) {
  startup_delay = false;
  return true;
}

Task checkTempTask(1000, checkTemp);
DelayRun clearDelayTask(5000, clearDelay);

void setup() {
  // Set up the nodes array
  Serial.begin(9600);
  Serial.println("Starting");
  
  startup_delay = true;
  for (int i = 0; i < MAX_DEVICE_COUNT; i++) {
    int pin = PUMP_OFFSET + i;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  for (int i = 0; i < MAX_DEVICE_COUNT; i++) {
    int pin = PUMP_OFFSET + i;
    Serial.print("Checking ");
    Serial.println(pin);
    digitalWrite(pin, HIGH);
    delay(5000);
    digitalWrite(pin, LOW);
    Serial.println("done");
  }

  for (int i = 0; i < DEVICE_COUNT; i++) {
    nodes[i].sensor = sensor_list+i;
    nodes[i].pump = PUMP_OFFSET + i;
    nodes[i].temp = 0.0;
    nodes[i].status = 0;
    nodes[i].time = 0;
  }
  devManager.begin();
  SoftTimer.add(&checkTempTask);
  clearDelayTask.startDelayed();
}

// vi:ft=cpp ai sw=2:
