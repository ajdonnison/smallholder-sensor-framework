/**
 * Version 2, uses a 4 digit 7-segment display
 * and MAX7219 display driver chip
 * along with both cooling and heating options
 * and programmable set points.
 */
#include <DallasTemperature.h>
#include <PciManager.h>
#include <OneWire.h>
#include <DelayRun.h>
#include <SoftTimer.h>
#include <Debouncer.h>
#include <LedControl.h>
#include <avr/eeprom.h>
#include "setup.h"

OneWire dataBus(ONE_WIRE_IF);
DallasTemperature devManager(&dataBus);
float temperature;
DeviceAddress thermometer;
boolean addressValid = false;
boolean relayOn = false;
boolean relay2On = false;

enum _set_mode {
  run_mode = 0,
  setup_mode,
  hc_mode,
  temp_mode,
  hys_mode,
  hold_mode
} 
set_mode;

_set_mode current_top_level;

const char * mode_list[] = {
  "RUN ",
  "5trt",
  "HC  ",
  "TC  ",
  "Diff",
  "5et "
};

const char * hc_modes[] = {
  "Heat",
  "Cool"
};

const char * msgs[] = {
  "    ",
  "----",
  "5trt",
  "Err "
};

#define DISPLAY_MSG_CLEAR 0
#define DISPLAY_MSG_WRITE 1
#define DISPLAY_MSG_START 2
#define DISPLAY_MSG_ERROR 3

struct _cfg {
  uint8_t set_point;
  uint8_t hysterisis;
  uint8_t mode;
  uint8_t changed;
} 
cfg;

LedControl ld(DATA_IN, CLK, CHIP_SELECT,1);

void displayString(const char * str) {
  for (int i = 0; i < 4; i++) {
    ld.setChar(0, 3-i, str[i] & 0x7f, str[i] & 0x80); 
  }
}

void displayMessage(int msg) {
  displayString(msgs[msg]);
}

void writeConfig() {
  uint32_t result;
  if (cfg.changed) {
    Serial.println("Write config");
    cfg.changed = 0xa5;
    displayMessage(DISPLAY_MSG_WRITE);
    memcpy(&result, &cfg, sizeof(uint32_t));
    eeprom_write_dword((uint32_t *)0, result);
    displayMessage(DISPLAY_MSG_CLEAR);
    cfg.changed = 0;
  }
}

void readConfig() {
  uint32_t result;
  result = eeprom_read_dword((uint32_t *)0);
  memcpy(&cfg, &result, sizeof(uint32_t));
  if (cfg.changed != 0xa5) {
    cfg.set_point = 30;
    cfg.mode = 1;
    cfg.hysterisis = 2;
    cfg.changed = 0xa5;
    writeConfig();
  }
  cfg.changed = 0;
}

void displayTemp(float tempC) {
  int temp;
  char str[4];

  temp = tempC * 100;
  //str[3] = temp % 10;
  str[3] = 'C';
  str[2] = ((temp + 5)/ 10) % 10;
  str[1] = ((temp / 100) % 10) | 0x80;
  str[0] = (temp / 1000) % 10;

  displayString((const char *)str);
}

/*
 * Setup protocol
 * Press Up button 2 seconds to go into setup
 * Press up/down button to cycle through options
 * Press Up button 2 seconds to select option
 * Same within options, use up/down buttons - up button for 2 seconds to save
 */
void showMode() {
  displayString(mode_list[current_top_level]);
}

boolean changeMode(Task * me) {
  switch (set_mode) {
  case run_mode: // In normal running mode, go to top-level setup
    set_mode = setup_mode;
    current_top_level = hc_mode;
    showMode();
    break;
  case setup_mode: // In setup mode, select current mode
    if (current_top_level == hold_mode) {
      set_mode = run_mode;
      writeConfig();
    } else {
      set_mode = current_top_level;
      showOptions();
    }
    break;
  case hc_mode: // In lower level modes, save current option
  case temp_mode:
  case hys_mode:
    set_mode = setup_mode;
    showMode();
    break;
  case hold_mode: // In hold mode, save all options
    set_mode = run_mode;
    writeConfig();
    break;
  }
  return true;
}

DelayRun startModeChange(SETUP_TIMER, changeMode);

void showOptions() {
  char str[5];
  switch (set_mode) {
  case hc_mode:
    displayString(hc_modes[cfg.mode]);
    break;
  case temp_mode:
    sprintf(str, "  %2d", cfg.set_point);
    displayString(str);
    break;
  case hys_mode:
    sprintf(str, "  %2d", cfg.hysterisis);
    displayString(str);
    break;
  }
}

void setMode(int inc) {
  switch (set_mode) {
  case setup_mode:
    switch (current_top_level) {
    case hold_mode:
      current_top_level = hc_mode;
      break;
    case hc_mode:
      current_top_level = temp_mode;
      break;
    case temp_mode:
      current_top_level = hys_mode;
      break;
    case hys_mode:
      current_top_level = hold_mode;
      break;

    }
    showMode();
    break;
  case hc_mode:
    cfg.mode = ! cfg.mode;
    cfg.changed = 1;
    showOptions();
    break;
  case temp_mode:
    if ((inc > 0 && cfg.set_point < 99)
      || (inc < 0 && cfg.set_point > 0)) {
      cfg.set_point += inc;
      cfg.changed = 1;
    }
    showOptions();
    break;
  case hys_mode:
    if ((inc > 0 && cfg.hysterisis < 99)
      || (inc < 0 && cfg.hysterisis > 0)) {
      cfg.hysterisis += inc;
      cfg.changed = 1;
    }
    showOptions();
    break;
  }
}

void upOn() {
  startModeChange.startDelayed();
}

void upOff(long unsigned int tm) {
  SoftTimer.remove(&startModeChange);
  if (tm < SETUP_TIMER) {
    setMode(1);
  }
}

void dnOn() {
  startModeChange.startDelayed();
}

void dnOff(long unsigned int tm) {
  SoftTimer.remove(&startModeChange);
  if (tm < SETUP_TIMER) {
    setMode(-1);
  }
}

void checkTemp(Task *me) {
  if (set_mode != run_mode) {
    return;
  }
  if (! addressValid) {
    if (!devManager.getAddress(thermometer, 0)) {
      addressValid = false;
      Serial.println("No valid thermometer devices");
      displayMessage(DISPLAY_MSG_ERROR);
      return;
    } 
    else {
      addressValid = true;
    }
  }
  devManager.requestTemperatures();
  temperature = devManager.getTempC(thermometer);
  displayTemp(temperature);
  Serial.println(temperature);
  if (temperature >= cfg.set_point) {
    if (cfg.mode && !relayOn) {
      digitalWrite(RELAY, HIGH);
      digitalWrite(INDICATOR, HIGH);
      relayOn = true;
      Serial.println("Turning cooler on");
    } 
    else if (!cfg.mode && relayOn) {
      digitalWrite(RELAY, LOW);
      digitalWrite(INDICATOR, LOW);
      relayOn = false;
      Serial.println("Turning relay off");
    }
  } 
  else if (temperature < (cfg.set_point - cfg.hysterisis)) {
    if (cfg.mode && relayOn) {
      digitalWrite(RELAY, LOW);
      digitalWrite(INDICATOR, LOW);
      Serial.println("Turning cooler off");
      relayOn = false;
    } 
    else if (!cfg.mode && !relayOn) {
      digitalWrite(RELAY, HIGH);
      digitalWrite(INDICATOR, HIGH);
      Serial.println("Turning heater on");
      relayOn = true;
    }
  }
}

void checkLight(Task *me)
{
  if (HIGH == digitalRead(BUTTON_LIGHT)) {
    if (! relay2On) {
      relay2On = true;
      digitalWrite(RELAY_2, HIGH);
    }
  } else {
    if (relay2On) {
      relay2On = false;
      digitalWrite(RELAY_2, LOW);
    }
  }
}

Task checkTempTask(1000, checkTemp);
Task checkLightButton(99, checkLight);
Debouncer upButton(BUTTON_UP, MODE_CLOSE_ON_PUSH, upOn, upOff);
Debouncer dnButton(BUTTON_DOWN, MODE_CLOSE_ON_PUSH, dnOn, dnOff);

void setup() {
  // Set up the nodes array
  Serial.begin(9600);
  Serial.println("Starting");

  pinMode(RELAY, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  pinMode(BUTTON_UP, INPUT);
  pinMode(BUTTON_DOWN, INPUT);
  pinMode(BUTTON_LIGHT, INPUT);
  pinMode(INDICATOR, OUTPUT);

  digitalWrite(RELAY, LOW);
  digitalWrite(RELAY_2, LOW);
  digitalWrite(INDICATOR, LOW);
  digitalWrite(BUTTON_UP, HIGH);
  digitalWrite(BUTTON_DOWN, HIGH);
  digitalWrite(BUTTON_LIGHT, HIGH);

  relayOn = false;
  devManager.begin();
  addressValid = false;
  readConfig();
  SoftTimer.add(&checkTempTask);
  SoftTimer.add(&checkLightButton);

  PciManager.registerListener(BUTTON_UP, &upButton);
  PciManager.registerListener(BUTTON_DOWN, &dnButton);

  // Handle the LED array
  ld.shutdown(0,false);
  ld.setIntensity(0,8);
  ld.setScanLimit(0,3);
  ld.clearDisplay(0);
  displayMessage(DISPLAY_MSG_START);
}
// vi:ft=cpp sw=2 ai:

