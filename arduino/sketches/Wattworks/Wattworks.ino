/*
 * "WattWorks" is the name of a commercial greywater system.
 * The controller that came with it was pretty basic and this
 * one has the benefit of beint able to log information and
 * also to be remotely controlled.
 * While originally designed for a particular system, it can
 * manage any system where water level detection is used to
 * control pump out.
 *
 * TODO:
 * - Add timer for daily flush out
 * - Change to use SoftTimer
 */
#include <XBee.h>
#include <SoftwareSerial.h>
#include <Saki.h>

/* Inputs are active low */
#define INPUT_ACTIVE LOW

/* Set up a few pins that we need. */
int lowSensor = 5;
int midSensor = 6;
int highSensor = 7;
int pumpRelay = 10;

/* Initialise statuses
 * We set these to an impossible value so that the first
 * check will report a status change.
 */
int lowStatus = -1;
int midStatus = -1;
int highStatus = -1;
boolean flushCommand = false;
boolean statusChange = false;
boolean pumpEnable = false;
bool remoteMode = false;

 /* Configurables */
long flushDelay = 0; // Seconds to keep pump running after low indication
long pumpTimeout = 1800; // Warn if pump on for longer than this

/* Support for times */
unsigned long flushTimer = 0;

/* Set up the manager using SerialSoftware on port 3/4 */
SakiManager manager("WW", 3, 1, true);
SoftwareSerial serialPort(3, 4);

/* Simple method to turn the pump on */
void startPump(boolean force = false) {
  pumpEnable = true;
  manager.setAlarm(pumpTimeout);
  digitalWrite(pumpRelay, LOW);
}

/* Turn the pump off, if we are being called from a remote device,
 also turn off any flush pending command */
void stopPump(boolean force = false) {
  pumpEnable = false;
  manager.clearAlarm();
  digitalWrite(pumpRelay, HIGH);
  if (force) {
    flushCommand = false;
  }
}

void updateConfig() {
  long value;
  SakiConfig * cfg;
  
  cfg = manager.getConfig();
  manager.configChanged = false;
  
  if ((value = cfg->get("PT")) != 0) {
    pumpTimeout = value;
  }
  if ((value = cfg->get("FD")) != 0) {
    flushDelay = value;
  }
  remoteMode = (bool)cfg->get("RM");
}

/* Send a message back to the controller with the current status.
 * We only send this on a status change */
void reportStatus(const char ** Msg) {
  Serial.print("2Status: ");
  Serial.print(lowStatus);
  Serial.print(":");
  Serial.print(midStatus);
  Serial.print(":");
  Serial.print(highStatus);
  Serial.print(":");
  Serial.println(pumpEnable);
  manager.setDigitalInput(0, lowStatus == INPUT_ACTIVE);
  manager.setDigitalInput(1, midStatus == INPUT_ACTIVE);
  manager.setDigitalInput(2, highStatus == INPUT_ACTIVE);
  manager.setDigitalOutput(0, pumpEnable);
  manager.report(Msg == NULL);
  Serial.println("Sent");
}


void setStatus(const char ** Msg) {
  int status;
  if (!strcmp(Msg[1], "O")) {
    status = atoi(Msg[2]);
    if (status) {
      Serial.println("PUMP ON");
      startPump(true);
      reportStatus(NULL);
    } else {
      Serial.println("PUMP OFF");
      stopPump(true);
      reportStatus(NULL);
    }
  }
}

void setError(char *msg) {
  char * buf;
  buf = (char *)malloc(strlen(msg)+4);
  sprintf(buf, "ER:%s", msg);
  manager.send(buf);
  free(buf);
}

/* Just check inputs and set flags - then decide if anything
 has changed */
void checkInputs() {
  int oldLow = lowStatus;
  int oldMid = midStatus;
  int oldHigh = highStatus;
  unsigned long now;
  lowStatus = digitalRead(lowSensor);
  midStatus = digitalRead(midSensor);
  highStatus = digitalRead(highSensor);
  if (oldLow != lowStatus || oldMid != midStatus || oldHigh != highStatus) {
    statusChange = true;
  } else if (pumpEnable) {
    if (manager.isAlarmed()) {
      stopPump(true);
      reportStatus(NULL);
      setError("PM");
    }
  }
}

/* The "business logic" for managing the pump. */
void controlPump() {
  unsigned long now;
  
  if (lowStatus != INPUT_ACTIVE) {
    stopPump(true);
  }
  if (flushCommand || highStatus == INPUT_ACTIVE) {
    startPump();
  }
  if (flushCommand == false && pumpEnable && remoteMode && midStatus != INPUT_ACTIVE) {
    stopPump();
  }
  // Final check, if we are called and the pump is still running after the timeout
  // we kill the pump and send an error.
  if (pumpEnable) {
    if (manager.isAlarmed()) {
      stopPump(true);
      reportStatus(NULL);
      setError("PM");
    }
  }
}

void setup() {
  SakiConfig * cfg;
  /* Set sensor inputs and pump output modes */
  manager.debug(false);  // Disable debug until serial is ready.
  pinMode(lowSensor, INPUT);
  pinMode(midSensor, INPUT);
  pinMode(highSensor, INPUT);
  pinMode(pumpRelay, OUTPUT);
  /* For inputs, set internal pullups so we can use a simple grounding switch */
  digitalWrite(lowSensor, HIGH);
  digitalWrite(midSensor, HIGH);
  digitalWrite(highSensor, HIGH);
  /* Set the pump off, just to be sure */
  digitalWrite(pumpRelay, HIGH);
  Serial.begin(9600);
  serialPort.begin(9600);
  Serial.println("0Starting...");
  manager.registerHandler("ST", &setStatus);
  manager.registerHandler("ST?", &reportStatus);
  manager.debug(true);
  manager.startClock(); /* Required for alarms */
  manager.start(serialPort);
  cfg = manager.getConfig();
  cfg->load();
  cfg->setDefault("RM", remoteMode ? 1 : 0);
  cfg->setDefault("PT", pumpTimeout);
  cfg->setDefault("FD", flushDelay);
  cfg->save();
  updateConfig();
  delay(100); // Don't start up immediately, give the XBee time to fire up.
  Serial.println("Going into loop...");
}


void loop() {
  manager.check();
  if (manager.configChanged) {
    updateConfig();
  }
  checkInputs();
  if (statusChange) {
    statusChange = false;
    controlPump();
    reportStatus(NULL);
  }
}

// vim:ft=cpp sw=2 ai expandtab:

