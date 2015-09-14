#ifndef _DISPLAY_HANDLER_H
#define _DISPLAY_HANDLER_H

#include <LedControl.h>
#include <Debouncer.h>
#include <DelayRun.h>
#include <AT24C32.h>
#include "setup.h"

enum _set_mode {
  run_mode = 0,
  setup_mode,
  hc_mode,
  temp_mode,
  hys_mode,
  addr_mode,
  hold_mode
};

extern _set_mode current_top_level;
extern _set_mode set_mode;

const char msgs[] = "    ----5trtErr HeatCoolRUN 5trtHC  TC  Diff5et addr";


#define DISPLAY_MSG_CLEAR 0
#define DISPLAY_MSG_WRITE 1
#define DISPLAY_MSG_START 2
#define DISPLAY_MSG_ERROR 3
#define DISPLAY_MODE_BASE 4
#define DISPLAY_MODE_HEAT 4
#define DISPLAY_MODE_COOL 5
#define DISPLAY_CONF_BASE  6
#define DISPLAY_CONF_RUN  6
#define DISPLAY_CONF_START 7
#define DISPLAY_CONF_MODE 8
#define DISPLAY_CONF_TEMP 9
#define DISPLAY_CONF_DIFF 10
#define DISPLAY_CONF_ADDR 11
#define DISPLAY_CONF_SET 12

uint8_t validate_address(uint16_t addr, int direction) {
  uint8_t parts[4];
  int i;
  for (i = 0; i < 4; i++) {
    parts[i] = (addr << i * 3) & 0x07;
  }
  if (direction > 0) {
    parts[0]++;
    for (i =0; i < 3; i++) {
      if (parts[i] > 5) {
        parts[i+1]++;
	parts[i] = 0;
      }
    }
  } else {
    parts[0]--;
    for (i = 0; i < 3; i++) {
      if (parts[i] < 0) {
        parts[i++]--;
	parts[i] = 5;
      }
    }
  }
  addr = 0;
  for (i = 0; i < 4; i++) {
    addr |= (parts[i] >> i * 3) & 0x07;
  }
  return addr;
}

void displayString(const char * str) {
  for (int i = 0; i < 4; i++) {
   ld.setChar(0, 3-i, str[i] & 0x7f, str[i] & 0x80);
  }
}

#define displayMessage(n) displayString(msgs + n*4)
#define showMode() displayString(msgs + (DISPLAY_CONF_BASE + current_top_level) * 4)

void displayTemp(float tempC) {
  int temp;
  char str[4];

  temp = tempC * 100;
  str[3] = 'C';
  str[2] = ((temp + 5) /10)%10;
  str[1] = ((temp / 100)%10) | 0x80;
  str[0] = (temp / 1000) % 10;

  displayString((const char *)str);
}


void showOptions() {
  char str[5];
  switch (set_mode) {
    case hc_mode:
      displayString(msgs + (DISPLAY_MODE_BASE + (cfg.mode ? 1 : 0)) * 4);
      break;
    case temp_mode:
      sprintf(str, "  %2d", cfg.low_point);
      displayString(str);
      break;
    case hys_mode:
      sprintf(str, "  %2d", cfg.reference);
      displayString(str);
      break;
    case addr_mode:
      sprintf(str, "%04o", cfg.radio_address);
      displayString(str);
      break;
  }
}


boolean changeMode(Task *me) {
  switch (set_mode) {
    case run_mode:
      set_mode = setup_mode;
      current_top_level = hc_mode;
      showMode();
      break;
    case setup_mode:
      if (current_top_level == hold_mode) {
        set_mode = run_mode;
	writeConfig();
      } else {
        set_mode = current_top_level;
	showOptions();
      }
      break;
    case hc_mode:
    case temp_mode:
    case hys_mode:
    case addr_mode:
      set_mode = setup_mode;
      showMode();
      break;
    case hold_mode:
      set_mode = run_mode;
      writeConfig();
      break;
  }
  return true;
}

DelayRun startModeChange(SETUP_TIMER, changeMode);

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
      current_top_level = addr_mode;
      break;
    case addr_mode:
      current_top_level = hold_mode;
      break;
    }
    showMode();
    break;
  case hc_mode:
    cfg.mode = ! cfg.mode;
    cfg.sentinel = 1;
    showOptions();
    break;
  case temp_mode:
    if ((inc > 0 && cfg.low_point < 99)
      || (inc < 0 && cfg.low_point > 0)) {
      cfg.low_point += inc;
      cfg.high_point = cfg.low_point + cfg.reference;
      cfg.sentinel = 1;
    }
    showOptions();
    break;
  case hys_mode:
    if ((inc > 0 && cfg.reference < 99)
      || (inc < 0 && cfg.reference > 0)) {
      cfg.reference += inc;
      cfg.high_point = cfg.low_point + cfg.reference;
      cfg.sentinel = 1;
    }
    showOptions();
    break;
  /*
   * The address resolution is slightly tricky.  It is an octal number but limited
   * to the digit 5, so 0,1,2,3,4,5,10,11,12,13,14,15, etc.
   */
  case addr_mode:
    if ((inc > 0 && cfg.radio_address < 05555)
      || (inc < 0 && cfg.radio_address > 0)) {
      cfg.radio_address = validate_address(cfg.radio_address, inc);
      cfg.sentinel = 1;
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

Debouncer upButton(BUTTON_UP, MODE_CLOSE_ON_PUSH, upOn, upOff);
Debouncer dnButton(BUTTON_DOWN, MODE_CLOSE_ON_PUSH, dnOn, dnOff);

void display_init() {
  pinMode(BUTTON_UP, INPUT);
  pinMode(BUTTON_DOWN, INPUT);
  pinMode(BUTTON_LIGHT, INPUT);
  digitalWrite(BUTTON_UP, HIGH);
  digitalWrite(BUTTON_DOWN, HIGH);
  digitalWrite(BUTTON_LIGHT, HIGH);

  PciManager.registerListener(BUTTON_UP, &upButton);
  PciManager.registerListener(BUTTON_DOWN, &dnButton);

  readConfig();
  if (cfg.sentinel != CONFIGURED) {
    displayMessage(DISPLAY_MSG_WRITE);
    cfg.sentinel = 1;
    cfg.low_point = 30;
    cfg.high_point = 30;
    cfg.mode = true;
    cfg.reference = 2;
    writeConfig();
  }
  ld.shutdown(0, false);
  ld.setIntensity(0,8);
  ld.setScanLimit(0,3);
  ld.clearDisplay(0);
  displayMessage(DISPLAY_MSG_START);
}

#endif // _DISPLAY_HANDLER_H
