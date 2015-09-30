/*
 * Pins in use for standard controller
 * These have been optimised for maximal
 * connectivity with other devices while
 * still leaving enough pins for I/O
 */
// 0,1 are used by serial connection.
#define	RELAY		2
#define RELAY_2		3
#define INDICATOR	4
#define ONE_WIRE_IF	5 // Used by Dallas temp sensors
#define CHIP_SELECT	6 // Display driver
#define DATA_IN		7 // Display driver
#define CLK		8 // Display driver
#define RADIO_CE	9
#define RADIO_CS	10
// MOSI/MISO/SCK are 11, 12, 13, used by radio
#define BUTTON_UP	A0
#define BUTTON_DOWN	A1
#define BUTTON_SET	A2
#define BATTERY_LEVEL	A3
// A4, A5 are used by RTC.
#define BUTTON_LIGHT	A6
