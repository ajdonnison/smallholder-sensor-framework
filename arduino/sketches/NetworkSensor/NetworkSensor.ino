/*
 * Generic sensor and relay output.
 * Currently supports 1 or 2 temperature sensors only
 * should be generalised to others.
 * Optionally supports a 7-segment 4 digit LED display
 * and two button control.
 * Can support heating or cooling modes, and either
 * absolute or relative (1 or 2 sensors).
 * Has two outputs, one controlled by time, the other
 * by the temperature settings.
 */
#include "pins.h"
#include "setup.h"
#include <Wire.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include <Time.h>
#if HAS_RTC
 #include <DS1307RTC.h>
#endif
#include <PciManager.h>
#include <SoftTimer.h>
#include "message.h"

struct _cfg {
  uint8_t sentinel;
  uint8_t low_point;
  uint8_t high_point;
  uint8_t reference;
  uint16_t low_time;
  uint16_t high_time;
  uint16_t radio_address;
  bool relay;
  bool mode;
  DeviceAddress temp_sensors[MAX_TEMP_SENSORS];
} cfg;

#if HAS_LED_DISPLAY
 #include <LedControl.h>
 LedControl ld(DATA_IN, CLK, CHIP_SELECT, 1);
 #include "display.h"
 _set_mode current_top_level;
 _set_mode set_mode;
#else
 #define displayTemp(s)
#endif

RF24 radio(RADIO_CE,RADIO_CS);
RF24Network network(radio);
#if HAS_EEPROM
 #include <AT24C32.h>
 AT24C32 eeprom(0);
#endif

bool haveMessage = false;
uint32_t my_counter = 0;
sensor_msg_t last_status;

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
#if DEBUG
    Serial.println(F("msg sent"));
#endif
  } else {
#if DEBUG
    Serial.println(F("msg send fail"));
#endif
  }
  // And now we request a network update
  networkScanTask((Task *)NULL);
}

void
sendTime(void)
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

#if DEBUG
void
printTime()
{
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.print(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print("/");
  Serial.print(month());
  Serial.print("/");
  Serial.println(year());
}
#else
#define printTime()
#endif

void
requestConfig(void) {
  message_t cmsg;

  cmsg.payload.config.item = 'a';
  sendMessage('r', &cmsg);
}

void
sendConfigItem(uint32_t item, uint32_t value) {
  message_t cmsg;
  cmsg.payload.config.item = item;
  cmsg.payload.config.value = value;
  sendMessage('C', &cmsg);
}

void
sendConfig() {
  // Send a series of config messages out.
  message_t cmsg;

  sendConfigItem('l', cfg.low_point);
  sendConfigItem('h', cfg.high_point);
  sendConfigItem('r', cfg.reference);
  sendConfigItem('s', cfg.low_time);
  sendConfigItem('e', cfg.high_time);
  sendConfigItem('m', cfg.mode);
}

void
networkScanTask(Task *me)
{
  message_t msg, s_msg;

  RF24NetworkHeader header;
  network.update();
  while (network.available()) {
    network.read(header, (void *)&msg, sizeof(msg));
    switch (header.type) {
      case 'r': // Request config
	sendConfig();
	break;
      case 't': // Request time
        sendTime();
	break;
      case 's': // Request status
	memcpy(&(s_msg.payload.sensor), &last_status, sizeof(sensor_msg_t));
	sendMessage('s', &s_msg);
	break;
      case 'c': // Config
        switch (msg.payload.config.item) {
	  case 't': // Timestamp
#if HAS_RTC
	    RTC.set(msg.payload.config.value);
#endif
	    setTime(msg.payload.config.value);
	    sendTime();
	    break;
	  case 'h': // High Value
	    cfg.high_point = msg.payload.config.value;
	    cfg.sentinel = 1;
	    writeConfig();
	    sendConfigItem('h', cfg.high_point);
	    break;
	  case 'l': // Low Value
	    cfg.low_point = msg.payload.config.value;
	    cfg.sentinel = 1;
	    writeConfig();
	    sendConfigItem('l', cfg.low_point);
	    break;
	  case 'r': // Reference
	    cfg.reference = msg.payload.config.value;
	    cfg.sentinel = 1;
	    writeConfig();
	    sendConfigItem('r', cfg.reference);
	    break;
	  case 's': // Start time
	    cfg.low_time = msg.payload.config.value;
	    cfg.sentinel = 1;
	    writeConfig();
	    sendConfigItem('s', cfg.low_time);
	    break;
	  case 'e': // End time
	    cfg.high_time = msg.payload.config.value;
	    cfg.sentinel = 1;
	    writeConfig();
	    sendConfigItem('e', cfg.high_time);
	    break;
	  case 'm': // Mode
	    cfg.mode = msg.payload.config.value;
	    cfg.sentinel = 1;
	    writeConfig();
	    sendConfigItem('m', cfg.mode);
	    break;
	  case 'a': // Address
	    cfg.radio_address = msg.payload.config.value;
	    cfg.sentinel = 1;
	    writeConfig();
	    sendConfigItem('a', cfg.radio_address);
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

#if HAS_LED_DISPLAY
  if (set_mode != run_mode) {
    return;
  }
#endif

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
#if DEBUG
  Serial.print(test);
  Serial.write(':');
  Serial.println(reference);
  sendTime();
#endif
  now = (hour() + TZ_OFFSET)%24 * 100 + minute();
#if HAS_TIMED_RELAY
  if (cfg.low_time < now && now < cfg.high_time) {
    digitalWrite(RELAY_2, HIGH);
    msg.payload.sensor.value_3 = 1;
  } else {
    digitalWrite(RELAY_2, LOW);
    msg.payload.sensor.value_3 = 0;
  }
#endif

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

#if DEBUG
  sendMessage('s', &msg);
#else
  if (msg.payload.sensor.value != last_status.value
    || msg.payload.sensor.value_2 != last_status.value_2
    || msg.payload.sensor.value_3 != last_status.value_3
    || msg.payload.sensor.value_4 != last_status.value_4) {
      sendMessage('s', &msg);
  }
#endif
  memcpy(&last_status, &(msg.payload.sensor), sizeof(sensor_msg_t));
}

#if DEBUG
void
printConfig(void)
{
  Serial.print("ADDR:");
  Serial.println(cfg.radio_address);
  Serial.print("Low Temp:");
  Serial.println(cfg.low_point);
  Serial.print("High Temp:");
  Serial.println(cfg.high_point);
  Serial.print("Reference:");
  Serial.println(cfg.reference);
  Serial.print("Low Time:");
  Serial.println(cfg.low_time);
  Serial.print("High Time:");
  Serial.println(cfg.high_time);
}
#else
#define printConfig()
#endif

void
readConfig(void)
{
#if HAS_EEPROM
  int read = eeprom.readBytes(0, (void *)&cfg, sizeof(cfg));
#else
#if DEBUG
  Serial.println("No EEPROM - returning random data");
#endif
#endif
}

void
writeConfig(void)
{
#if HAS_EEPROM
  if (cfg.sentinel) {
    cfg.sentinel = CONFIGURED;
    eeprom.writeBytes(0, (void *)&cfg, sizeof(cfg));
    cfg.sentinel = 0;
  }
#endif
}

Task networkScan(RADIO_ADDRESS + NETWORK_LOOP_MS, networkScanTask);
Task sensorScan(RADIO_ADDRESS + SENSOR_LOOP_MS, sensorScanTask);

void setup(void)
{
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
  pinMode(INDICATOR, OUTPUT);
  digitalWrite(INDICATOR, LOW);
#if HAS_TIMED_RELAY
  pinMode(RELAY_2, OUTPUT);
  digitalWrite(RELAY_2, LOW);
#endif
  Serial.begin(9600);
  Serial.println(F("Starting"));
  // First, check that we have time
#if HAS_RTC
  setSyncProvider(RTC.get);
#endif
  timeStatus();
#if HAS_RTC
  while (! RTC.chipPresent()) {
    timeStatus_t t;
    Serial.println(F("Failed to find RTC ... Check connections"));
    t = timeStatus();
    if (t != timeSet) {
      if (t == timeNotSet) {
        Serial.println(F("Time NOT SET"));
      } else {
        Serial.println(F("Time Needs SYNC"));
      }
    }
    delay(4000);
  }
#endif

  SPI.begin();

  // check our configuration
  tempSensors.begin();
  readConfig();
  if (cfg.sentinel != CONFIGURED) {
    cfg.radio_address = RADIO_ADDRESS;
    cfg.relay = RADIO_RELAY;
    configureTemp();
  }
  if (cfg.radio_address > 05555) {
    cfg.radio_address = RADIO_ADDRESS; // Sanity Check.
  }
  printConfig();
  radio.begin();
  network.begin(CHANNEL, cfg.radio_address);
  if (cfg.sentinel != CONFIGURED) {
#if DEBUG
    Serial.println(F("Request Config"));
#endif
    requestConfig();
  }
  /* Initialise the last status message */
  last_status.type = 1;
  last_status.value = 0;
  last_status.value_2 = 0;
  last_status.value_3 = 0;
  last_status.value_4 = 0;
  last_status.adjust = 0;

  SoftTimer.add(&networkScan);
  SoftTimer.add(&sensorScan);
#if HAS_LED_DISPLAY
  set_mode = run_mode;
  current_top_level = run_mode;
  display_init();
#endif
}

// vim:ft=cpp ai sw=2:
