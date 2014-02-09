/*
 * This is a quick and nasty single-temperature controller.
 * The values chosen were for a poultry scalder used to
 * loosen the feathers prior to plucking.
 * While I could have done it without a micro controller,
 * I had all the components on the shelf.
 */
#include <DallasTemperature.h>
#include <OneWire.h>
#include <DelayRun.h>
#include <SoftTimer.h>
#include <BlinkTask.h>

#define ONE_WIRE_IF 10
#define INDICATOR 13
#define HEATER 9
#define CUT_OUT_TEMP 65
#define HYSTERISIS 2

OneWire dataBus(ONE_WIRE_IF);
DallasTemperature devManager(&dataBus);
float temperature;
DeviceAddress thermometer;
boolean addressValid = false;
boolean heaterOn = false;

BlinkTask errorLed(INDICATOR, 200);
BlinkTask heating(INDICATOR, 1000);

void checkTemp(Task *me) {
  if (! addressValid) {
    if (!devManager.getAddress(thermometer, 0)) {
      addressValid = false;
      Serial.println("No valid thermometer devices");
      errorLed.start();
      return;
    } else {
      addressValid = true;
      errorLed.stop();
    }
  }
  devManager.requestTemperatures();
  temperature = devManager.getTempC(thermometer);
  Serial.println(temperature);
  if (temperature >= CUT_OUT_TEMP) {
    if (heaterOn) {
      digitalWrite(HEATER, LOW);
      heating.stop();
      heaterOn = false;
      Serial.println("Turning heater off");
    }
    digitalWrite(INDICATOR, HIGH);
  } else if (temperature < (CUT_OUT_TEMP - HYSTERISIS)) {
    if (!heaterOn) {
      digitalWrite(HEATER, HIGH);
      Serial.println("TUrning heater on");
      heating.start();
      heaterOn = true;
    }
  }
}

Task checkTempTask(1000, checkTemp);

void setup() {
  // Set up the nodes array
  Serial.begin(9600);
  Serial.println("Starting");
  
  pinMode(HEATER, OUTPUT);
  digitalWrite(HEATER, LOW);
  pinMode(INDICATOR, OUTPUT);
  digitalWrite(INDICATOR, LOW);
  heaterOn = false;
  devManager.begin();
  addressValid = false;
  SoftTimer.add(&checkTempTask);
}
// vi:ft=cpp sw=2 ai:
