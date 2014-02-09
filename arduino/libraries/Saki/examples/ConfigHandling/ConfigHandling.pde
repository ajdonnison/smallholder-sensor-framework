/*
 * Example showing the config management of the Saki management library
 */

/* We need to ensure we have the XBee library and SoftwareSerial.
   This is because config handlers only make sense with remote
   config updates */
#include <XBee.h>
#include <SoftwareSerial.h>
#include <Saki.h>

/* Set up the software serial, used to communicate with the XBee */
SoftwareSerial ser(5,6);
/* Create a manager instance. Arguments are:
   - Name of the instance, should be kept short, preferably 2 characters
   - Number of inputs
   - Number of outputs
   - Flag to indicate if the module is to be remotely managed (i.e. other
     modules can send requests)
*/
SakiManager manager("TEST", 1, 1, true);

/* Set up some configurable values */
long configItem1 = 100;
long configItem2 = 200;
bool configFlag1 = true;

void updateConfig() {
  long value;
  SakiConfig * cfg = manager.getConfig();
  // clear the config flag
  manager.configChanged = false;
  if ((value = cfg->get("I1")) != 0) {
    configItem1 = value;
  }
  if ((value = cfg->get("I2")) != 0) {
    configItem2 = value;
  }
  configFlag1 = cfg->getBool("F1");
  cfg->print();
}

void setup() {
  SakiConfig * cfg;

  Serial.begin(9600);
  ser.begin(9600);
  manager.start(ser);
  cfg = manager.getConfig();  // Get a config instance
  cfg->load();	// Grab config from EEPROM
  cfg->setDefault("I1", configItem1); // Set the config if it isn't already set
  cfg->setDefault("I2", configItem2);
  cfg->setDefault("F1", configFlag1);
  cfg->save();	// Save back to EEPROM - note only written if it has changed
  cfg->print();  // Print out to serial port the config items and values.
}

void loop() {
  manager.check();	// Check if there are any messages waiting
  /* If a config message comes in it will change the config and write it
   * to EEPROM (only if there is a change to values to reduce the
   * number of unnecessary writes).  It then sets a flag so we can
   * query the config and update our local variables.
   */
  if (manager.configChanged) {
    updateConfig();
  }
  delay(100);
}
