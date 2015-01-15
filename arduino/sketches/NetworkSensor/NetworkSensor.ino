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
#include "message.h"

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
sendMessage(int type, message_t * msg)
{
  RF24NetworkHeader hdr(0, type);
  if (network.write(hdr, (void *)msg, sizeof(message_t))) {
    Serial.println(F("msg sent"));
  } else {
    Serial.println(F("msg send fail"));
  }
}

void
printTime()
{
  message_t tm;

  tm.payload.time.year = year();
  tm.payload.time.month = month();
  tm.payload.time.day = day();
  tm.payload.time.hour = hour();
  tm.payload.time.minute = minute();
  tm.payload.time.second = second();

  sendMessage('t', &tm);
}

void
requestConfig(void) {
  message_t cmsg;

  cmsg.payload.config.item = 'a';
  sendMessage('r', &cmsg);
}

void
printConfig(void) {
  // Send a series of config messages out.
  message_t cmsg;

  cmsg.payload.config.item = 'l';
  cmsg.payload.config.value = cfg.low_point;
  sendMessage('C', &cmsg);
  cmsg.payload.config.item = 'h';
  cmsg.payload.config.value = cfg.high_point;
  sendMessage('C', &cmsg);
  cmsg.payload.config.item = 'r';
  cmsg.payload.config.value = cfg.reference;
  sendMessage('C', &cmsg);
  cmsg.payload.config.item = 's';
  cmsg.payload.config.value = cfg.low_time;
  sendMessage('C', &cmsg);
  cmsg.payload.config.item = 'e';
  cmsg.payload.config.value = cfg.high_time;
  sendMessage('C', &cmsg);
}

void
networkScanTask(Task *me)
{
  message_t msg;

  RF24NetworkHeader header;
  network.update();
  if (network.available()) {
    RF24NetworkHeader header;
    network.read(header, (void *)&msg, sizeof(msg));
    switch (header.type) {
      case 'r': // Request config
         printConfig();
	 break;
      case 'c': // Config
        switch (msg.payload.config.item) {
	  case 't': // Timestamp
	    RTC.set(msg.payload.config.value);
	    setTime(msg.payload.config.value);
	    break;
	  case 'h': // High Value
	    cfg.high_point = msg.payload.config.value;
	    cfg.sentinel = 1;
	    writeConfig();
	    break;
	  case 'l': // Low Value
	    cfg.low_point = msg.payload.config.value;
	    cfg.sentinel = 1;
	    writeConfig();
	    break;
	  case 'r': // Reference
	    cfg.reference = msg.payload.config.value;
	    cfg.sentinel = 1;
	    writeConfig();
	    break;
	  case 's': // Start time
	    cfg.low_time = msg.payload.config.value;
	    cfg.sentinel = 1;
	    writeConfig();
	    break;
	  case 'e': // End time
	    cfg.high_time = msg.payload.config.value;
	    cfg.sentinel = 1;
	    writeConfig();
	    break;
	}
    }
  }
}

void
sensorScanTask(Task *me)
{
  float test;
  float reference;
  uint16_t now;

  if (set_mode != run_mode) {
    return;
  }

  message_t msg;

  msg.payload.sensor.type = 1;
  msg.id = 1;

  tempSensors.requestTemperatures();
  test = tempSensors.getTempC(cfg.temp_sensors[0]);
  msg.payload.sensor.value = test * 10;

  if (MAX_TEMP_SENSORS > 1) {
    reference = tempSensors.getTempC(cfg.temp_sensors[1]);
  } else {
    reference = test + cfg.reference;
  }
  msg.payload.sensor.value_2 = reference * 10;
  Serial.print(test);
  Serial.write(':');
  Serial.println(reference);

  printTime();
  now = (hour() + TZ_OFFSET)%24 * 100 + minute();
  if (cfg.low_time < now && now < cfg.high_time) {
    digitalWrite(RELAY_2, HIGH);
    msg.payload.sensor.value_3 = 1;
  } else {
    digitalWrite(RELAY_2, LOW);
    msg.payload.sensor.value_3 = 0;
  }

  displayTemp(test);

  if (test < cfg.low_point) {
    digitalWrite(RELAY,cfg.mode ? LOW : HIGH);
    digitalWrite(INDICATOR, cfg.mode ? LOW : HIGH);
    msg.payload.sensor.value_4 = cfg.mode ? 0 : 1;
  }
  else if (test >= cfg.high_point && test >= reference && (test - reference) >= cfg.reference) {
    digitalWrite(RELAY, cfg.mode ? HIGH : LOW);
    digitalWrite(INDICATOR, cfg.mode ? HIGH : LOW);
    msg.payload.sensor.value_4 = cfg.mode ? 1 : 0;
  }
  sendMessage('s', &msg);
}

void
readConfig(void)
{
  int read = eeprom.readBytes(0, (void *)&cfg, sizeof(cfg));
}

void
writeConfig(void)
{
  if (cfg.sentinel) {
    cfg.sentinel = CONFIGURED;
    eeprom.writeBytes(0, (void *)&cfg, sizeof(cfg));
    cfg.sentinel = 0;
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

  SPI.begin();

  // check our configuration
  tempSensors.begin();
  readConfig();
  if (cfg.sentinel != CONFIGURED) {
    cfg.radio_address = RADIO_ADDRESS;
    cfg.relay = RADIO_RELAY;
  }

  radio.begin();
  network.begin(CHANNEL, cfg.radio_address);
  if (cfg.sentinel != CONFIGURED) {
    requestConfig();
  }
  SoftTimer.add(&networkScan);
  SoftTimer.add(&sensorScan);
  set_mode = run_mode;
  current_top_level = run_mode;
  display_init();
}

// vim:ft=cpp ai sw=2:
