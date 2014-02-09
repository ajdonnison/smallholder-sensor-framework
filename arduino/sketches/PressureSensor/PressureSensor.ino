#include <XBee.h>
#include <SoftwareSerial.h>
#include <Saki.h>
#include <SoftTimer.h>

#define PRESSURE_SENSOR A0
#define XB_TX 10
#define XB_RX 11
#define IND_LOW 5
#define IND_MED 7
#define IND_HIGH 9

long pressure, depth;
// Diameter in mm
long diameter = 0;
long volume = 0;
long height = 0;

SoftwareSerial ser(XB_RX, XB_TX);
SakiManager manager("PS", 3, 0, true);

void reportStatus(const char **Msg) {
  manager.setAnalogInput(0, depth, 0);
  manager.setAnalogInput(1, volume, 0);
  manager.setAnalogInput(2, pressure, 0);
  manager.report(Msg == NULL);
}

void updateConfig() {
  long value;
  SakiConfig * cfg = manager.getConfig();
  manager.configChanged = false;
  if ((value = cfg->get("DI")) != 0) {
    diameter = value;
  }
  if ((value = cfg->get("HI")) != 0) {
    height = value;
  }
}

/*
 * Uses a direct reading pressure sensor.  This sensor
 * produces a voltage on the Analog Input pin that is
 * defined by:
 * Vin = 0.2Vcc * (P + 0.5)
 * Where
 * Vin = Voltage presented to analog input
 * Vcc = Supply voltage to pressure sensor
 * P = Pressure in atmospheres (101.325 kPa)
 *
 * For water depth we need to know the fluid characteristics
 * however we assume water is roughly at 1000kg/m3 for fresh
 * water so from:
 * P = pqh
 * we get a change of 9.8kPa per metre of depth.
 *
 * So we need to convert a number between 0 and 1023 to a
 * pressure, and hence a depth.
 *
 * This gives us:
 *
 * 101.325kPa = 1024/5
 * So we get 1 tick = 495Pa = roughly 50mm which is our resolution
 * 
 * To get the pressure we need to remove the 0.5 atmospheres
 */
 
void checkPressure(Task *me) {
  double radius;
  int raw_value = analogRead(PRESSURE_SENSOR);
  double real_pressure = (495 * (long)raw_value) - 49500;
  Serial.println(real_pressure);
  double real_depth = real_pressure / 98.0;
  pressure = real_pressure / 1000.0;  // Pressure in kPa
  depth = real_depth; // Depth in cm
  if (diameter) {
    radius = (double)diameter / 200.0; // Get radius in decimetres
    volume = (3.141592653 * radius * radius * real_depth / 10.0); // Volume in litres
  }
  digitalWrite(IND_HIGH, LOW);
  digitalWrite(IND_MED, LOW);
  digitalWrite(IND_LOW, LOW);
  if (depth > (height * 0.6)) {
    digitalWrite(IND_HIGH, HIGH);
  } else if (depth > (height * 0.2)) {
    digitalWrite(IND_MED, HIGH);
  } else {
    digitalWrite(IND_LOW, HIGH);
  }
  reportStatus(NULL);
  Serial.print(raw_value);
  Serial.print("P:");
  Serial.print(real_pressure);
  Serial.print(":");
  Serial.print(pressure);
  Serial.print(" D:");
  Serial.print(real_depth);
  Serial.print(":");
  Serial.println(depth);
  
}

void checkManager(Task *me) {
  manager.check();
  if (manager.configChanged) {
    updateConfig();
  }
}

Task checkPressureTask(10000, checkPressure);
Task checkManagerTask(100, checkManager);

void setup() {
  pinMode(IND_LOW, OUTPUT);
  pinMode(IND_MED, OUTPUT);
  pinMode(IND_HIGH, OUTPUT);
  digitalWrite(IND_LOW, LOW);
  digitalWrite(IND_MED, LOW);
  digitalWrite(IND_HIGH, HIGH);
  
  SakiConfig * cfg;
  manager.debug(false);  
  Serial.begin(9600);
  ser.begin(9600);
  manager.registerHandler("ST?", &reportStatus);
  manager.debug(true);
  manager.start(ser);
  cfg = manager.getConfig();
  cfg->load();
  cfg->setDefault("HI", 210);
  cfg->setDefault("DI", 2400);
  cfg->save();
  updateConfig();
  manager.send("Starting");
  SoftTimer.add(&checkPressureTask);
  SoftTimer.add(&checkManagerTask);
}


/*
# vim:ai ft=cpp sw=2 expandtab:
*/
