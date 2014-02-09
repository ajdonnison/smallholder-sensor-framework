/*
 * Part of the Saki manager.
 *
 * Register a message handler.  In this case we provide two
 * handlers.  One for an "ON" command, one for an "OFF" command.
 * Test setup has two inputs, on and off, and a single output,
 * a LED.  The message handlers allow a remote execution of the
 * same functions.
 *
 */

#include <XBee.h>
#include <SoftwareSerial.h>
#include <Saki.h>

/* Define a few ports to use */
#define LED_OUT 13
#define ON_SWITCH 7
#define OFF_SWITCH 8
#define SER_TX 5
#define SER_RX 4

/* Set up the SoftwareSerial instance to communicate with the XBee */
SoftwareSerial ser(SER_RX, SER_TX);

/* Set up a manager instance, call it TST with 2 inputs, one output and remotely manageable */
SakiManager manager("TST", 2, 1, true);

/* Create a message handler for our ON condition */
void OnHandler(const char ** msg) {
  digitalWrite(LED_OUT, HIGH);
}

/* Create a message handler for our OFF condition */
void OffHandler(const char ** msg) {
  digitalWrite(LED_OUT, LOW);
}

/* Handle local inputs */
void checkInputs() {
  if (digitalRead(ON_SWITCH)) {
    digitalWrite(LED_OUT, HIGH);
  } else if (digitalRead(OFF_SWITCH)) {
    digitalWrite(LED_OUT, LOW);
  }
}

/* Set up */
void setup() {
  Serial.begin(9600);
  ser.begin(9600);
  pinMode(LED_OUT, OUTPUT);
  pinMode(ON_SWITCH, INPUT);
  pinMode(OFF_SWITCH, INPUT);
  /* Register our message handlers */
  manager.registerHandler("ON", &OnHandler);
  manager.registerHandler("OFF", &OffHandler);
  manager.start(ser);
}

void loop() {
  manager.check();
  checkInputs();
}
