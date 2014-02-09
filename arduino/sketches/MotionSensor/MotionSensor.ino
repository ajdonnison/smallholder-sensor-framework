/*
 Motion sensor controlled light.

 This uses cheap motion sensors that provide a high logic level
 when triggered.  The sensors have the time control turned to the
 minimum so that we just get a pulse that we use to trigger a local
 timer rather than rely on the motion sensor to do that.  This gives
 us the ability to combine with other sensors (like the headlight
 sensor) to provide a unified approach.

 This version uses:

 - 2 x PIR motion sensors ($3.43 for two)
 - 2 x phototransistor ($3.90 for 10)
 - 1 x Relay module ($6.50 for 5)
 - Arduino Pro Mini
 - Xbee Series 2 router

 One of the phototransistors is mounted on the top of the main unit
 to detect ambient light.  Output is inhibited if it isn't dark enough.

 The second phototransistor is mounted in a short tube set horizontally
 at headlight level in the carport.  When the car enters with headlights
 on it triggers the light.  The two PIRs are set at the back
 door and the door from the deck to the carport.

 The output relay uses a logic 0 (low level) to switch on, hence
 the HIGH used to turn it off.

 Any change in status is reported via the XBee, and levels and timer
 values can be changed via the XBee.

 We use the inbuilt serial port for debug, and the SoftwareSerial library
 to talk to the XBee.

 */
#include <XBee.h>
#include <SoftwareSerial.h>
#include <Saki.h>

#define MOTION_1 5
#define MOTION_2 6
#define LIGHT_1 A0
#define LIGHT_2 A1
#define LED_OUTPUT 10

/* Set values to impossible levels so that the initial check
   will cause statuses to be set correctly */
int motion1 = -1;
int motion2 = -1;
int light1 = -1;
int light2 = -1;
int dark1 = -1;
int dark2 = -1;

int ledStatus = 0;

/* Configurables */
int darkValue = 200;
int lightValue = 600;
int onTime = 60;
int onTime2 = 90;

// Need to declare this
void reportStatus(const char ** Msg);

SakiManager manager("MS", 4, 1, true);
SoftwareSerial serialPort(3,4);

void
checkInputs(bool defer = false) {
  int oldMotion1, oldMotion2, oldLight1, oldLight2, oldDark1, oldDark2;
  bool alarmed;
  oldMotion1 = motion1;
  oldMotion2 = motion2;
  oldLight1 = light1;
  oldLight2 = light2;
  oldDark1 = dark1;
  oldDark2 = dark2;

  motion1 = digitalRead(MOTION_1);
  motion2 = digitalRead(MOTION_2);
  light1 = analogRead(LIGHT_1);
  light2 = analogRead(LIGHT_2);
  dark1 = (light1 < darkValue);
  dark2 = (light2 > lightValue);
  alarmed = manager.isAlarmed(true);

  if (motion1 != oldMotion1 || motion2 != oldMotion2
  || dark1 != oldDark1 || dark2 != oldDark2 || alarmed) {
    if ((ledStatus && alarmed) || ! dark1) {
      ledStatus = 0;
      digitalWrite(LED_OUTPUT, HIGH);
      manager.clearAlarm(); // Only relevent with dark1
    }
    if (dark1 && (motion1 || motion2 || dark2)) {
      ledStatus = 1;
      digitalWrite(LED_OUTPUT, LOW);
      manager.setAlarm(dark2 ? onTime2 : onTime, true);
    } 
    if (!defer) {
      reportStatus(NULL);
    }
  }
}

void updateStatus(const char ** Msg) {
  int status;
  if (!strcmp(Msg[1], "O")) {
    status = atoi(Msg[2]);
    if (status) {
      // Turn on light
      manager.setAlarm(onTime, true);
      digitalWrite(LED_OUTPUT, LOW);
      ledStatus = 1;
    } else {
      manager.clearAlarm();
      ledStatus = 0;
      digitalWrite(LED_OUTPUT, HIGH);
    }
    reportStatus(NULL);
  }
}

void reportStatus(const char ** Msg) {
  if (Msg != NULL) {
    Serial.print("REQUEST ");
    checkInputs(true);
  }
  Serial.print("STATUS: ");
  Serial.print(motion1);
  Serial.print(":");
  Serial.print(motion2);
  Serial.print(":");
  Serial.print(light1);
  Serial.print(":");
  Serial.print(light2);
  Serial.print(":");
  Serial.println(ledStatus);
  manager.setDigitalInput(0, motion1);
  manager.setDigitalInput(1, motion2);
  manager.setAnalogInput(2, light1, 0);
  manager.setAnalogInput(3, light2, 0);
  manager.setDigitalOutput(0, ledStatus);
  manager.report(Msg == NULL);
}

void updateConfig() {
  long value;
  SakiConfig * cfg = manager.getConfig();
  // clear the config flag
  manager.configChanged = false;
  if ((value = cfg->get("T1")) != 0) {
    onTime = value;
  }
  if ((value = cfg->get("T2")) != 0) {
    onTime2 = value;
  }
  if ((value = cfg->get("DK")) != 0) {
    darkValue = value;
  }
  if ((value = cfg->get("HL")) != 0) {
    lightValue = value;
  }
}

void setup() {
  SakiConfig * cfg;
  manager.debug(false);
  pinMode(MOTION_1, INPUT);
  pinMode(MOTION_2, INPUT);
  pinMode(LED_OUTPUT, OUTPUT);
  digitalWrite(LED_OUTPUT, HIGH);
  Serial.begin(9600); 
  serialPort.begin(9600);
  Serial.println("Starting...");
  manager.registerHandler("ST?", &reportStatus);
  manager.registerHandler("ST", &updateStatus);
  manager.debug(true);
  manager.startClock(); /* If we don't get time set remotely we still need the clock running */
  manager.start(serialPort);
  cfg = manager.getConfig();
  cfg->load();
  cfg->setDefault("T1", (long)onTime);
  cfg->setDefault("T2", (long)onTime2);
  cfg->setDefault("DK", (long)darkValue);
  cfg->setDefault("HL", (long)lightValue);
  cfg->save();
  updateConfig();
  cfg->print();
  delay(100);
  Serial.println("Looping...");
}

void loop() {
  manager.check();
  if (manager.configChanged) {
    updateConfig();
  }
  checkInputs();
  delay(100);
}

/*
vim:ai sw=2 ft=cpp expandtab:
*/
