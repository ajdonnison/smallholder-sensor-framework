#include <DallasTemperature.h>
#include <OneWire.h>
#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include <Time.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <PciManager.h>
#include <SoftTimer.h>
#include <AT24C32.h>
#include "setup.h"
#define zeroFill(x) { int y = x / 10; Serial.write('0'+y); Serial.write('0'+x%10); }
#define hexify(x) { char hex[16] = { '0', '1', '2', '3', '4', '5', '6' ,'7', \
'8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };\
int y = x / 16; \
Serial.write(hex[y%16]); \
Serial.write(hex[x%16]); }

struct _cfg {
  uint8_t sentinel;
  uint8_t radio_address;
  DeviceAddress temp_sensors[MAX_TEMP_SENSORS];
  uint8_t low_point;
  uint8_t high_point;
  uint8_t reference;
  uint16_t low_time;
  uint16_t high_time;
  bool relay;
  bool mode;
} cfg;

#define CONFIGURED 0xda

#ifdef HAS_LED_DISPLAY
 #include <LedControl.h>
 LedControl ld(DATA_IN, CLK, CHIP_SELECT, 1);
 #include "display.h"
 _set_mode current_top_level;
 _set_mode set_mode;
#else
 #define checkTemp()
#endif

RF24 radio(RADIO_CE,RADIO_CS);
RF24Network network(radio);
AT24C32 eeprom(0);

bool haveMessage = false;
uint32_t my_counter = 0;


/*
 * Need to find out how many sensors we have on the 1-wire
 * network and to enumerate them and display the temperature
 * so that the user can identify which is which.
 * Every time we recall the sensors by bus ID it should return
 * the same device each time.
 */
OneWire oneWire(ONE_WIRE_IF);
DallasTemperature tempSensors(&oneWire);

void
configureTemp(void)
{
  float temp;
  int sensor_count;
  Serial.println(F("Checking for temperature sensors"));
  sensor_count = tempSensors.getDeviceCount();
  Serial.print(sensor_count);
  Serial.println(" Sensors found");
  tempSensors.requestTemperatures();
  for (int i = 0; i < MAX_TEMP_SENSORS && i < sensor_count; i++) {
    tempSensors.getAddress(cfg.temp_sensors[i], i);
    temp = tempSensors.getTempC(cfg.temp_sensors[i]);
    Serial.print(i);
    Serial.write(' ');
    Serial.println(temp);
  }
}

void
configureTime(void)
{
  unsigned long set = 0L;
  const unsigned long DEFAULT_TIME = 1400000000;

  Serial.println(F("Time is not set"));
  while (set < DEFAULT_TIME) {
    Serial.println(F("Please enter time as seconds in epoch"));
    while (!Serial.available()) ;
    set = Serial.parseInt();
  }
  RTC.set(set);
  setTime(set);
}

void
printTime()
{
  Serial.print(year());
  Serial.print("-");
  zeroFill(month());
  Serial.print("-");
  zeroFill(day());
  Serial.print(" ");
  zeroFill(hour());
  Serial.print(":");
  zeroFill(minute());
  Serial.print(":");
  zeroFill(second());
  Serial.println();
}

void
doConfigure(void) {
  int val;

  cfg.radio_address = 0;
  while ( ! cfg.radio_address) {
    Serial.println(F("Radio Address"));
    while (!Serial.available()) ;
    cfg.radio_address = Serial.parseInt();
  }
  Serial.println();
  Serial.println(F("Enable relay? (y/n)"));
  while (!Serial.available()) ;
  val = Serial.read();
  cfg.relay = (val == 'y' || val == 'Y');
  configureTemp();
  Serial.println(F("Low temperature"));
  while (!Serial.available());
  cfg.low_point = Serial.parseInt();
  Serial.println(F("High temperature"));
  while (!Serial.available());
  cfg.high_point = Serial.parseInt();
  Serial.println(F("Temperature difference"));
  while (!Serial.available());
  cfg.reference = Serial.parseInt();
  Serial.println(F("Start time"));
  while (!Serial.available());
  cfg.low_time = Serial.parseInt();
  Serial.println(F("End time"));
  while (!Serial.available());
  cfg.high_time = Serial.parseInt();
  cfg.mode = false;
  cfg.sentinel = CONFIGURED;
  eeprom.writeBytes(0, (void *)&cfg, sizeof(cfg));
  cfg.sentinel = 0;
}

void
printConfig(void) {
  Serial.print(F("Radio Address: "));
  Serial.println(cfg.radio_address, OCT);
  Serial.print(F("Mode: "));
  if (cfg.relay) {
    Serial.println(F("Relay"));
  } else {
    Serial.println(F("Leaf"));
  }
  for (int i = 0; i < MAX_TEMP_SENSORS; i++) {
    Serial.print(F("Temperature "));
    Serial.print(i);
    Serial.print(F(": "));
    for (int j = 0; j< 8; j++) {
      hexify(cfg.temp_sensors[i][j]);
    }
    Serial.println();
  }
  Serial.print(F("Low point: "));
  Serial.println(cfg.low_point);
  Serial.print(F("High point: "));
  Serial.println(cfg.high_point);
  Serial.print(F("Reference difference: "));
  Serial.println(cfg.reference);
}

void
networkScanTask(Task *me)
{
  RF24NetworkHeader header;
  network.update();
  if (network.available()) {
    RF24NetworkHeader header;
    network.peek(header);
    // network.read(header, (void *)&payload, sizeof(payload));
  }
}

void
sensorScanTask(Task *me)
{
  float test;
  float reference;
  uint16_t now;
  struct {
    uint32_t ms;
    uint32_t counter;
  } out_msg;

  if (set_mode != run_mode) {
    return;
  }

  tempSensors.requestTemperatures();
  test = tempSensors.getTempC(cfg.temp_sensors[0]);
  if (MAX_TEMP_SENSORS > 1) {
    reference = tempSensors.getTempC(cfg.temp_sensors[1]);
  } else {
    reference = test + cfg.reference;
  }
  Serial.print(test);
  Serial.write(':');
  Serial.println(reference);

  printTime();
  now = (hour() + TZ_OFFSET)%24 * 100 + minute();
  if (cfg.low_time < now && now < cfg.high_time) {
    digitalWrite(RELAY_2, HIGH);
  } else {
    digitalWrite(RELAY_2, LOW);
  }

  displayTemp(test);

  if (test < cfg.low_point) {
    Serial.println(F("Test point below low cutout"));
    digitalWrite(RELAY,cfg.mode ? LOW : HIGH);
    digitalWrite(INDICATOR, cfg.mode ? LOW : HIGH);
  }
  else if (test >= cfg.high_point && test >= reference && (test - reference) >= cfg.reference) {
    Serial.println(F("High point reached"));
    digitalWrite(RELAY, cfg.mode ? HIGH : LOW);
    digitalWrite(INDICATOR, cfg.mode ? HIGH : LOW);
  }
  RF24NetworkHeader msg_hdr(0);
  out_msg.counter = ++my_counter;
  out_msg.ms = millis();
    
  if (network.write(msg_hdr, (void *)&out_msg, sizeof(out_msg))) {
    Serial.println(F("SENT OK"));
  } else {
    Serial.println(F("Failed TX"));
  }
}

Task networkScan(100, networkScanTask);
Task sensorScan(5000, sensorScanTask);

void setup(void)
{
  pinMode(RELAY, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  pinMode(INDICATOR, OUTPUT);
  digitalWrite(RELAY, LOW);
  digitalWrite(RELAY_2, LOW);
  digitalWrite(INDICATOR, LOW);
  Serial.begin(9600);
  Serial.println(F("Starting"));
  // First, check that we have time
  setSyncProvider(RTC.get);
  timeStatus();
  while (! RTC.chipPresent()) {
    Serial.println(F("Failed to find RTC ... Check connections"));
    delay(4000);
  }

  if (timeStatus() != timeSet) {
    configureTime();
  } else {
    Serial.println(F("Time already set"));
  }
  SPI.begin();

  // check our configuration
  tempSensors.begin();
  int read = eeprom.readBytes(0, (void *)&cfg, sizeof(cfg));
  if (cfg.sentinel != CONFIGURED) {
    doConfigure();
  }
  printConfig();

  radio.begin();
  network.begin(CHANNEL, cfg.radio_address);
  SoftTimer.add(&networkScan);
  SoftTimer.add(&sensorScan);
  set_mode = run_mode;
  current_top_level = run_mode;
  display_init();
}

// vim:ft=cpp ai sw=2:
