/*
 * Brooder control v0.1.
 *
 * Designed to create a simple temperature controlled environment
 * for newly hatched chicks, ducklings, keets and other young
 * birds.
 *
 * Uses common-anode 7 segment LED display (2 digits) using
 * 7447 BCD to 7 segment decoder/driver for display output.
 *
 * Input is a DHT-22 combined temp/humidity sensor.
 * 
 * This version does not yet support light and humidity control.
 *
 * In the simplest form this just controls a heater element when
 * the ambient temperature drops below a threshold.
 *
 * Future versions will include the ability to control a light,
 * either with a timer or with an external light sensor.
 * Additionally a fan for reducing humidity may be of use.
 * We are running short of pins though, so it may require
 * refactoring if we get too smart.  Most of the pins are
 * required for the display (4 pins for value, 2 pins for
 * addressing and one for blanking control = 7 pins).
 * Options could include an I2C or 1-Wire display, or a second
 * controller to manage the display.
 */
#include <PciManager.h>
#include <SoftTimer.h>
#include <Debouncer.h>
#include <DHT22.h>
#include <EEPROM.h>

/* Inputs */
#define SET_BUTTON 2
#define UP_BUTTON 3
#define DN_BUTTON 4
#define TEMP_SENSOR 5
#define LIGHT_SENSOR 6

/* Outputs */
#define LED_1_SELECT 7
#define LED_2_SELECT 8
#define LED_BLANK 9
#define BCD_0 A0
#define BCD_1 A1
#define BCD_2 A2
#define BCD_3 A3
#define HEATER 10
#define LIGHT 11

/* Various controls */
#define SHOW_TEMP 1
#define SHOW_HUMID 2
#define ACTIVE LOW
#define INACTIVE HIGH

boolean inSetup = false;
boolean showDigit = true;
int setTemp;
int currentTemp = 0;
int currentHumid = 0;
boolean displayIndicator = false;
int displayMode = SHOW_TEMP;
DHT22 tempSensor(TEMP_SENSOR);

void displayBCD(int value) {
  digitalWrite(BCD_0, (value & 0x01) == 0x01 ? HIGH : LOW);
  digitalWrite(BCD_1, (value & 0x02) == 0x02 ? HIGH : LOW);
  digitalWrite(BCD_2, (value & 0x04) == 0x04 ? HIGH : LOW);
  digitalWrite(BCD_3, (value & 0x08) == 0x08 ? HIGH : LOW);
}

void showDisplay(Task *me) {
  int val;
  if (inSetup) {
    val = setTemp;
  } else {
    if (displayMode == SHOW_TEMP) {
      val = currentTemp / 10;
    } else {
      val = currentHumid / 10;
    }
  }
  if (displayIndicator) {
    digitalWrite(LED_BLANK, LOW);
    digitalWrite(LED_1_SELECT, LOW);
    digitalWrite(LED_2_SELECT, HIGH);
    displayIndicator = false;
    displayBCD(val % 10);
    if (showDigit) {
      digitalWrite(LED_BLANK, HIGH);
    }
  } else {
    digitalWrite(LED_BLANK, LOW);
    displayIndicator = true;
    digitalWrite(LED_2_SELECT, LOW);
    digitalWrite(LED_1_SELECT, HIGH);
    displayBCD(val / 10);
    if (showDigit) {
      digitalWrite(LED_BLANK, HIGH);
    }
  }
}

void checkTemp(Task *me) {
  int checkVal;
  DHT22_ERROR_t Err;
  char buf[16];
  Serial.println("Checking Temp");
  if ((Err = tempSensor.readData()) != DHT_ERROR_NONE) {
    Serial.print("Error ");
    Serial.println(Err);
    return;
  }
  currentTemp = tempSensor.getTemperatureCInt();
  currentHumid = tempSensor.getHumidityInt();
  checkVal = currentTemp / 10;
  sprintf(buf, "%d.%dC %d.%d%%", currentTemp/10, currentTemp%10, currentHumid/10, currentHumid%10);
  Serial.println(buf);
  if (checkVal >= setTemp) {
    digitalWrite(HEATER, INACTIVE);
  } else {
    digitalWrite(HEATER, ACTIVE);
  }
}

void setOn() {
  int oldTemp;
  Serial.print("Setup: ");
  Serial.print(inSetup);
  Serial.print(" ");
  Serial.println(setTemp);
  if (inSetup) {
    inSetup = false;
    showDigit = true;
    oldTemp = (int)EEPROM.read(0);
    if (oldTemp != setTemp) {
      EEPROM.write(0, (byte)(setTemp&0xff));
    }
  } else {
    inSetup = true;
  }
}

void upOn() {
  Serial.println("Up Pressed");
  if (inSetup) {
    setTemp++;
  } else {
    displayMode = SHOW_TEMP;
  }
}

void dnOn() {
  Serial.println("Down Pressed");
  if (inSetup) {
    setTemp--;
  } else {
    displayMode = SHOW_HUMID;          
  }
}

void toggleDisplay(Task *me) {
  if (inSetup) {
    showDigit ^= 1;
  }
}

/* Setup button handlers */
Debouncer setButton(SET_BUTTON, MODE_CLOSE_ON_PUSH, setOn, NULL);
Debouncer upButton(UP_BUTTON, MODE_CLOSE_ON_PUSH, upOn, NULL);
Debouncer dnButton(DN_BUTTON, MODE_CLOSE_ON_PUSH, dnOn, NULL);
Task displayTask(10, showDisplay);
Task checkTempTask(15000, checkTemp);
Task toggleDisplayTask(200, toggleDisplay);

void setup() {
  pinMode(SET_BUTTON,INPUT);
  pinMode(UP_BUTTON, INPUT);
  pinMode(DN_BUTTON, INPUT);
  pinMode(TEMP_SENSOR, INPUT);
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(BCD_0, OUTPUT);
  pinMode(BCD_1, OUTPUT);
  pinMode(BCD_2, OUTPUT);
  pinMode(BCD_3, OUTPUT);
  pinMode(LED_1_SELECT, OUTPUT);
  pinMode(LED_2_SELECT, OUTPUT);
  pinMode(LED_BLANK, OUTPUT);
  pinMode(HEATER, OUTPUT);
  pinMode(LIGHT, OUTPUT);
  
  digitalWrite(SET_BUTTON, HIGH);
  digitalWrite(UP_BUTTON, HIGH);
  digitalWrite(DN_BUTTON, HIGH);
  digitalWrite(BCD_0, LOW);
  digitalWrite(BCD_1, LOW);
  digitalWrite(BCD_2, LOW);
  digitalWrite(BCD_3, LOW);
  digitalWrite(LED_1_SELECT, LOW);
  digitalWrite(LED_2_SELECT, LOW);
  digitalWrite(LED_BLANK, LOW);
  digitalWrite(HEATER, INACTIVE);
  digitalWrite(LIGHT, LOW);
  Serial.begin(9600);
  Serial.println("Starting");
  showDigit = true;
  setTemp = (int)EEPROM.read(0);
  if (setTemp > 99) {
    setTemp = 32;
    EEPROM.write(0, (byte)(setTemp&0xff));
  }
  /* do an initial temp/humidity reading */
  delay(2500);
  
  PciManager.registerListener(SET_BUTTON, &setButton);
  PciManager.registerListener(UP_BUTTON, &upButton);
  PciManager.registerListener(DN_BUTTON, &dnButton);
  
  /* Set up the display tasks */
  SoftTimer.add(&displayTask);
  SoftTimer.add(&checkTempTask);
  SoftTimer.add(&toggleDisplayTask);
}
// vim:ai sw=2 expandtab ft=cpp:
