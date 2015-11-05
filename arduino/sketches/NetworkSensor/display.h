#ifndef _DISPLAY_HANDLER_H
#define _DISPLAY_HANDLER_H

#include <LedControl.h>
#include <Debouncer.h>
#include <DelayRun.h>
#include <AT24C32.h>
#include "setup.h"
#include <avr/pgmspace.h>

enum _set_mode {
  run_mode = 0,
  setup_mode,
  hc_mode,
  temp_mode,
  hys_mode,
  addr_mode,
  time_hour_mode,
  time_minute_mode,
  low_hour_mode,
  low_minute_mode,
  high_hour_mode,
  high_minute_mode,
  hold_mode,
  off_the_end
};

extern _set_mode current_top_level;
extern _set_mode set_mode;

const PROGMEM char msgs[] = "    ----5trtErr HeatCoolRUN 5trtHC  TC  DiffaddrT HrT -NL HrL -NH HrH -n5et ";


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
#define DISPLAY_CONF_TIME_HR 12
#define DISPLAY_CONF_TIME_MN 13
#define DISPLAY_CONF_LOW_HR 14
#define DISPLAY_CONF_LOW_MN 15
#define DISPLAY_CONF_HIGH_HR 16
#define DISPLAY_CONF_HIGH_MN 17
#define DISPLAY_CONF_SET 18

void displayString(const char * str) {
  for (int i = 0; i < 4; i++) {
   ld.setChar(0, 3-i, str[i] & 0x7f, str[i] & 0x80);
  }
}

void displayString_P(const char * PROGMEM str) {
  char c;
  for (int i = 0; i < 4; i++) {
    c = pgm_read_byte(str+i);
    ld.setChar(0, 3-i, c & 0x7f, c & 0x80);
  }
}

void displayNumber(const char * format, int number) {
  char str[5];
  sprintf(str, format, number % 10000);
#if DEBUG
  Serial.print(format);
  Serial.print(":");
  Serial.print(number);
  Serial.print(":");
  Serial.println(str);
#endif
  displayString(str);
}

#define displayMessage(n) displayString_P(msgs + n*4)
#define showMode() displayString_P(msgs + (DISPLAY_CONF_BASE + current_top_level) * 4)

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
      displayString_P(msgs + (DISPLAY_MODE_BASE + (cfg.mode ? 1 : 0)) * 4);
      break;
    case temp_mode:
      displayNumber("  %2d", cfg.low_point);
      break;
    case hys_mode:
      displayNumber("  %2d", cfg.reference);
      break;
    case addr_mode:
      displayNumber("%04o", cfg.radio_address & 0xfff);
      break;
    case time_hour_mode:
      displayNumber("  %02d", hour());
      break;
    case time_minute_mode:
      displayNumber("  %02d", minute());
      break;
    case low_hour_mode:
      displayNumber("  %02d", cfg.low_time / 100);
      break;
    case low_minute_mode:
      displayNumber("  %02d", cfg.low_time % 100);
      break;
    case high_hour_mode:
      displayNumber("  %02d", cfg.high_time / 100);
      break;
    case high_minute_mode:
      displayNumber("  %02d", cfg.high_time % 100);
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
    case time_hour_mode:
    case time_minute_mode:
    case low_hour_mode:
    case low_minute_mode:
    case high_hour_mode:
    case high_minute_mode:
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

int increment_hour(int hr, int inc) {
  hr += inc;
  if (hr >= 24) hr = 0;
  if (hr < 0) hr = 23;
  return hr;
}

int increment_minute(int min, int inc) {
  min += inc;
  if (min >= 60) min = 0;
  if (min < 0) min = 59;
  return min;
}

void setMode(int inc) {
  int hr, min;
  switch (set_mode) {
  case setup_mode:
    current_top_level = static_cast<_set_mode>(static_cast<int>(current_top_level) + inc);
    if (inc > 0 && current_top_level > hold_mode) {
      current_top_level = hc_mode;
    }
    if (inc < 0 && current_top_level < hc_mode) {
      current_top_level = hold_mode;
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
  case addr_mode:
    cfg.radio_address += inc;
    if (cfg.radio_address > 05555 || cfg.radio_address < 0) {
      cfg.radio_address = 0;
    }
    cfg.sentinel = 1;
    showOptions();
    break;
  case time_hour_mode:
  case time_minute_mode:
    int sec, dy, mth, yr;
    sec = 0;
    dy = day();
    mth = month();
    yr = year();
    hr = hour();
    min = minute();
    if (set_mode == time_hour_mode) {
     hr = increment_hour(hr, inc);
    } else {
     min  = increment_minute(min, inc);
    }
    setTime(hr, min, sec, dy, mth, yr);
#if HAS_RTC
    RTC.set(now());
#endif
    showOptions();
    break;
  case low_hour_mode:
    hr = increment_hour(cfg.low_time / 100, inc);
    min = cfg.low_time % 100;
    cfg.low_time = (hr * 100) + min;
    cfg.sentinel = 1;
    showOptions();
    break;
  case low_minute_mode:
    hr = cfg.low_time / 100;
    min = increment_minute(cfg.low_time % 100, inc);
    cfg.low_time = (hr * 100) + min;
    cfg.sentinel = 1;
    showOptions();
    break;
  case high_hour_mode:
    hr = increment_hour(cfg.high_time / 100, inc);
    min = cfg.high_time % 100;
    cfg.high_time = (hr * 100) + min;
    cfg.sentinel = 1;
    showOptions();
    break;
  case high_minute_mode:
    hr = cfg.high_time / 100;
    min = increment_minute(cfg.high_time % 100, inc);
    cfg.high_time = (hr * 100) + min;
    cfg.sentinel = 1;
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
    configureTemp();
    cfg.sentinel = 1;
    cfg.low_point = 30;
    cfg.high_point = 30;
    cfg.low_time = 700;
    cfg.high_time = 1900;
    cfg.mode = false;
    cfg.reference = 0;
    writeConfig();
  }
  ld.shutdown(0, false);
  ld.setIntensity(0,8);
  ld.setScanLimit(0,3);
  ld.clearDisplay(0);
  displayMessage(DISPLAY_MSG_START);
}

#endif // _DISPLAY_HANDLER_H
