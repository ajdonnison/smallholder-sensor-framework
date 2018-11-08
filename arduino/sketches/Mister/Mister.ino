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
#include <EEPROM.h>

#define ONE_WIRE_IF 2
#define PUMP_OFFSET 3
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
unsigned char ASSOC_MODE = 0;
byte currentMenu = 0;
int currentSensor = 0;

typedef struct st_devlist {
  float temp;
  int time;
  uint8_t status;
  uint8_t fill[3];
} DEVICE_LIST;

typedef struct _devs_t {
  DeviceAddress sensor;
  uint8_t pump;
  uint8_t enabled;
  uint16_t fill;
} devs_t;

struct _cfg {
  int devcount;
  devs_t sensors[MAX_DEVICE_COUNT];
  float max_temp;
} cfg;


DEVICE_LIST nodes[MAX_DEVICE_COUNT];

void readConfig(void) {
  EEPROM.get(0, cfg);
  // Now we build our devices list
  for (int i = 0; i < MAX_DEVICE_COUNT && i < cfg.devcount; i++) {
    nodes[i].status = 0;
    nodes[i].time = 0;
    nodes[i].temp = 0;
  }
}

void writeConfig(void) {
  EEPROM.put(0, cfg);
}

void findSensors(void) {
  int sensor_count;

  // Clear sensor list
  for (int i = 0; i < MAX_DEVICE_COUNT; i++) {
    cfg.sensors[i].enabled = 0;
  }

  sensor_count = devManager.getDeviceCount();
  for (int i = 0; i < MAX_DEVICE_COUNT && i < sensor_count; i++) {
    devManager.getAddress(cfg.sensors[i].sensor, i);
    cfg.sensors[i].enabled = 1;
  }
  cfg.devcount = sensor_count;
}

boolean stopPump(Task *me) {
  for (int i = 0; i < cfg.devcount && i < MAX_DEVICE_COUNT; i++) {
    if (nodes[i].status == 1) {
      dprint(F("Pump "));
      dprint(cfg.sensors[i].pump + 1 - PUMP_OFFSET);
      dprintln(" Off");
      digitalWrite(cfg.sensors[i].pump, LOW);
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
  
  for (int i = 0; i < cfg.devcount && i < MAX_DEVICE_COUNT; i++) {
    nodes[i].temp = devManager.getTempC(cfg.sensors[i].sensor);
    dprint(F("Temp "));
    dprint(cfg.sensors[i].pump + 1 - PUMP_OFFSET);
    dprint(": ");
    dprintln(nodes[i].temp);
    switch (nodes[i].status) {
      case 2:
        if (nodes[i].time-- < 0) {
          nodes[i].status = 0;
        }
        break;
      case 0:
        if (nodes[i].temp > cfg.max_temp) {
          dprint(F("Turning on pump "));
          dprintln(cfg.sensors[i].pump + 1 - PUMP_OFFSET);
          nodes[i].status = 1;
          digitalWrite(cfg.sensors[i].pump, HIGH);
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

void showSensor(void) {
  devManager.requestTemperatures();
  devs_t device = cfg.sensors[currentSensor];
  float temp = devManager.getTempC(device.sensor);
  Serial.print(currentSensor + 1);
  Serial.print(F(" ADDR:"));
  for (int i = 0; i < sizeof(DeviceAddress); i++) {
    Serial.print(device.sensor[i], HEX);
  }
  Serial.print(F(" PUMP:"));
  Serial.print(device.pump + 1 - PUMP_OFFSET);
  Serial.print(F(" TEMP:"));
  Serial.println(temp);
}

void showStatus() {
  if (TESTMODE || ASSOC_MODE) {
    devManager.requestTemperatures();
  }
  Serial.print(F("Max Temp: "));
  Serial.println(cfg.max_temp);
  for (int i = 0; i < MAX_DEVICE_COUNT; i++) {
    if ((TESTMODE || ASSOC_MODE) && i < cfg.devcount) {
      nodes[i].temp = devManager.getTempC(cfg.sensors[i].sensor);
    }
    Serial.print(F("Pump "));
    Serial.print(cfg.sensors[i].pump + 1 - PUMP_OFFSET);
    Serial.print(F(" Pin:"));
    Serial.print(cfg.sensors[i].pump);
    Serial.print(F(" Temp:"));
    Serial.print(nodes[i].temp);
    Serial.print(F(" Sensor Addr:"));
    for (int x = 0; x < sizeof(DeviceAddress); x++) {
      Serial.print(cfg.sensors[i].sensor[x], HEX);
    }
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
  float newmax = 0;

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
        case 'm':
          Serial.println(F("Set Max temp:"));
          if ((newmax = Serial.parseFloat()) > 0) {
            Serial.print(F("Setting max temp to "));
            Serial.println(newmax);
            cfg.max_temp = newmax;
            writeConfig();
          }
          break;
        case 'a':
          currentMenu = cmd;
          ASSOC_MODE = 1;
          Serial.println(F("Association mode"));
          break;
	default:
	  Serial.println(F("Menu options:"));
	  Serial.println(F("d - Toggle DEBUG"));
	  Serial.println(F("t - Test mode"));
	  Serial.println(F("s - Show Status"));
	  break;
      }
      break;
    case 'a':
      switch (cmd) {
        case 'x':
          ASSOC_MODE = 0;
          currentMenu = 0;
	  writeConfig();
	  readConfig();
          Serial.println(F("Run mode"));
          break;
        case 's':
	  findSensors();
	  currentSensor = 0;
	  Serial.print(F("Found "));
	  Serial.print(cfg.devcount);
	  Serial.println(F(" sensors"));
	  showSensor();
          break;
        case 'n':
	  if (++currentSensor >= cfg.devcount) currentSensor=cfg.devcount-1;
	  showSensor();
          break;
        case 'p':
	  if (--currentSensor < 0) currentSensor = 0;
	  showSensor();
          break;
        default:
          if (cmd >= '1' && cmd < ('1' + MAX_DEVICE_COUNT)) {
	    int pin = cmd - '1' + PUMP_OFFSET;
	    if (pin != cfg.sensors[currentSensor].pump) {
	      cfg.sensors[currentSensor].pump = pin;
	    }
	    showSensor();
          }
          else {
            Serial.println(F("Associate options"));
            Serial.println(F("s - Scan devices"));
            Serial.println(F("n - Next device"));
            Serial.println(F("p - Previous device"));
            Serial.println(F("x - Exit"));
          }
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
	  if (cmd >= '1' && cmd < ('1' + MAX_DEVICE_COUNT)) {
	    int i = cmd - '1';
            int j = -1;
            for (int x = 0; x < MAX_DEVICE_COUNT; x++) {
              if (i == (cfg.sensors[x].pump - PUMP_OFFSET))
                j = i;
                break;
            }
            if (j == -1) {
              Serial.println(F("Cannot find pump"));
              break;
            }
	    if (nodes[j].status == 1) {
	      Serial.println(F("Turning off pump "));
	      Serial.print(cfg.sensors[j].pump + 1 - PUMP_OFFSET);
	      nodes[j].status = 0;
	      digitalWrite(cfg.sensors[j].pump, LOW);
	    } else {
	      Serial.println(F("Turning on pump "));
	      Serial.print(cfg.sensors[j].pump + 1 - PUMP_OFFSET);
	      nodes[j].status = 1;
	      digitalWrite(cfg.sensors[j].pump, HIGH);
	    }
	  }
	  else {
	    Serial.println(F("Test options:"));
	    Serial.println(F("1-9 - toggle pump status"));
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
  Serial.begin(9600);
  Serial.println("Starting");
  Serial.setTimeout(5000);
  
  startup_delay = true;
  heartbeat.start();
  for (int i = 0; i < MAX_DEVICE_COUNT; i++) {
    int pin = PUMP_OFFSET + i;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  // Check config, set up nodes array
  readConfig();
  devManager.begin();
  SoftTimer.add(&checkTempTask);
  SoftTimer.add(&menuHandler);
  // Now see if we need to drop into config mode
  if (cfg.devcount <= 0 || cfg.devcount > MAX_DEVICE_COUNT) {
    cfg.max_temp = MAX_TEMP;
    currentMenu = 'a';
    ASSOC_MODE = 1;
    findSensors();
    showStatus();
    currentSensor = 0;
    showSensor();
  }
  clearDelayTask.startDelayed();
}

// vi:ft=cpp ai sw=2:
