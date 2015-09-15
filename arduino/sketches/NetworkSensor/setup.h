// Include for handling hardware specific pinouts
/*
// Original set
#define ONE_WIRE_IF      10
#define RELAY            12
#define RELAY_2          11
#define INDICATOR        13
#define CHIP_SELECT       2
#define DATA_IN           3
#define CLK               5
#define BUTTON_UP         6
#define BUTTON_DOWN       4
#define BUTTON_LIGHT      7
*/

// Redefined to include radio and RTC
// 0,1 are used by serial connection.
#define	RELAY		2
#define RELAY_2		3
#define INDICATOR	4
#define ONE_WIRE_IF	5
#define CHIP_SELECT	6
#define DATA_IN		7
#define CLK		8
#define RADIO_CE	9
#define RADIO_CS	10
// MOSI/MISO/SCK are 11, 12, 13, used by radio
#define BUTTON_UP	A0
#define BUTTON_DOWN	A1
#define BUTTON_SET	A2
#define BATTERY_LEVEL	A3
// A4, A5 are used by RTC.
#define BUTTON_LIGHT	A6

#define SETUP_TIMER    2000

#define CHANNEL		90
#define MAX_TEMP_SENSORS	1
#define RADIO_ADDRESS	01
#define RADIO_RELAY	true

#define HAS_LED_DISPLAY 1
#define TZ_OFFSET 10
#define HAS_EEPROM 1
#define HAS_RTC 1

// some forward definitions to assist in include ordering
void readConfig(void);
void writeConfig(void);
