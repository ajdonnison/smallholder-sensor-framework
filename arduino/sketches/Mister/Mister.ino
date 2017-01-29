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
#include <BlinkTask.h>

#define ONE_WIRE_IF 2
#define PUMP_OFFSET 3
#define DEVICE_COUNT 2
#define MAX_DEVICE_COUNT 4
#define MAX_TEMP 36.0
#define MIN_WAIT 240
#define ON_TIME 30
#define INDICATOR 13

#define dprintln(x) if (DEBUG) Serial.println(x)
#define dprint(x) if (DEBUG) Serial.print(x)

OneWire dataBus(ONE_WIRE_IF);
DallasTemperature devManager(&dataBus);
boolean startup_delay = true;

// Flags
unsigned char DEBUG = 0;
unsigned char TESTMODE = 0;
byte currentMenu = 0;

typedef struct st_devlist {
  DeviceAddress * sensor;
  int pump;
  int status;
  float temp;
  int time;
} DEVICE_LIST;


DEVICE_LIST nodes[MAX_DEVICE_COUNT];

DeviceAddress  sensor_list[] = {
  { 0x28, 0x22, 0x5d, 0x97, 0x04, 0x00, 0x00, 0x46 },
  // { 0x28, 0x79, 0x47, 0x97, 0x04, 0x00, 0x00, 0x51 },
  { 0x28, 0xc7, 0xff, 0x97, 0x04, 0x00, 0x00, 0x34 }
};

boolean stopPump(Task *me) {
  for (int i = 0; i < DEVICE_COUNT; i++) {
    if (nodes[i].status == 1) {
      dprint(F("Pump "));
      dprint(nodes[i].pump);
      dprintln(" Off");
      digitalWrite(nodes[i].pump, LOW);
      nodes[i].status = 2;
      nodes[i].time = MIN_WAIT;
    }
  }
  return false;
}

DelayRun stopPumpTask(ON_TIME * 1000, stopPump);

void checkTemp(Task *me) {
  boolean show_indicator = false;
  devManager.requestTemperatures();
  if (startup_delay || TESTMODE) {
    return;
  }
  
  for (int i = 0; i < DEVICE_COUNT; i++) {
    nodes[i].temp = devManager.getTempC(*(nodes[i].sensor));
    dprint(F("Temp "));
    dprint(nodes[i].pump);
    dprint(": ");
    dprintln(nodes[i].temp);
    switch (nodes[i].status) {
      case 2:
        if (nodes[i].time-- < 0) {
          nodes[i].status = 0;
        }
        break;
      case 0:
        if (nodes[i].temp > MAX_TEMP) {
          dprint(F("Turning on pump "));
          dprintln(nodes[i].pump);
          nodes[i].status = 1;
          digitalWrite(nodes[i].pump, HIGH);
          stopPumpTask.startDelayed();
        }
        break;
    }
    if (nodes[i].status) {
      show_indicator = true;
    }
  }
  digitalWrite(INDICATOR, show_indicator ? HIGH : LOW);
}

BlinkTask heartbeat(INDICATOR, 500);

boolean clearDelay(Task *me) {
  startup_delay = false;
  heartbeat.stop();
  digitalWrite(INDICATOR, LOW);
  return true;
}

void showStatus() {
  if (TESTMODE) {
    devManager.requestTemperatures();
  }
  for (int i = 0; i < MAX_DEVICE_COUNT; i++) {
    if (TESTMODE && i < DEVICE_COUNT) {
      nodes[i].temp = devManager.getTempC(*(nodes[i].sensor));
    }
    Serial.print(F("Pump "));
    Serial.print(i);
    Serial.print(F(" Pin:"));
    Serial.print(nodes[i].pump);
    Serial.print(F(" Temp:"));
    Serial.print(nodes[i].temp);
    Serial.print(F(" Status:"));
    switch (nodes[i].status) {
      case 0:
        Serial.println(F("STOPPED"));
	break;
      case 1:
        Serial.println(F("RUNNING"));
	break;
      case 2:
        Serial.print(F("WAITING "));
	Serial.println(nodes[i].time);
	break;
    }
  }
}

void handleCommand(int cmd) {
  switch (currentMenu) {
    case 0:
      switch (cmd) {
        case 'd':
	  Serial.print(F("Turning DEBUG "));
	  if (DEBUG) {
	    DEBUG = 0;
	    Serial.println(F("off"));
	  }
	  else {
	    DEBUG = 1;
	    Serial.println(F("on"));
	  }
	  break;
	case 't':
	  currentMenu = cmd;
	  TESTMODE = 1;
	  Serial.println(F("Test mode"));
	  break;
	case 's':
	  showStatus();
	  break;
	default:
	  Serial.println(F("Menu options:"));
	  Serial.println(F("d - Toggle DEBUG"));
	  Serial.println(F("t - Test mode"));
	  Serial.println(F("s - Show Status"));
	  break;
      }
      break;
    case 't':
      switch (cmd) {
	case 's':
	  showStatus();
	  break;
	case 'x':
	  TESTMODE = 0;
	  currentMenu = 0;
	  Serial.println(F("Run mode"));
	  break;
	default:
	  if (cmd >= '0' && cmd < ('0' + DEVICE_COUNT)) {
	    int i = cmd - '0';
	    if (nodes[i].status == 1) {
	      Serial.println(F("Turning off pump "));
	      Serial.print(i);
	      nodes[i].status = 0;
	      digitalWrite(nodes[i].pump, LOW);
	    } else {
	      Serial.println(F("Turning on pump "));
	      Serial.print(i);
	      nodes[i].status = 1;
	      digitalWrite(nodes[i].pump, HIGH);
	    }
	  }
	  else {
	    Serial.println(F("Test options:"));
	    Serial.println(F("0-9 - toggle pump status"));
	    Serial.println(F("s - Show Status"));
	    Serial.println(F("x - Exit test mode"));
	  }
	  break;
      }
      break;
  }
}

// Handle keyboard input
void kint(Task *me) {
  int cmd;
  if (Serial.available()) {
    handleCommand(Serial.read());
  }
}

Task checkTempTask(1000, checkTemp);
Task menuHandler(100, kint);
DelayRun clearDelayTask(6000, clearDelay);

void setup() {
  // Set up the nodes array
  Serial.begin(9600);
  Serial.println("Starting");
  
  startup_delay = true;
  heartbeat.start();
  for (int i = 0; i < MAX_DEVICE_COUNT; i++) {
    int pin = PUMP_OFFSET + i;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  for (int i = 0; i < MAX_DEVICE_COUNT; i++) {
    if (i < DEVICE_COUNT) {
      nodes[i].sensor = sensor_list+i;
    }
    nodes[i].pump = PUMP_OFFSET + i;
    nodes[i].temp = 0.0;
    nodes[i].status = 0;
    nodes[i].time = 0;
  }
  devManager.begin();
  SoftTimer.add(&checkTempTask);
  SoftTimer.add(&menuHandler);
  clearDelayTask.startDelayed();
}

// vi:ft=cpp ai sw=2:
