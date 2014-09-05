#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>

RF24 radio(CE_PIN,CS_PIN);
RF24Network network(radio);

void setup(void)
{
  SPI.begin();
  radio.begin();
  network.begin(CHANNEL, ADDRESS);
}

void loop(void)
{
  network.update();
  while (network.available()) {
  }
}

// vim:ft=cpp ai sw=2:
