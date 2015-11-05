/**
 * Configurable items for sensor
 */

/*
 * SETUP_TIMER is the backoff required
 * at startup after devices are initialised
 */
#define SETUP_TIMER    2000

/*
 * CHANNEL is the radio channel to use - all
 * radios in the same network require the same
 * channel.
 */
#define CHANNEL		90
/*
 * RADIO_ADDRESS is structured as an octal
 * number indicating the place in the network.
 * 00 is the base station
 * 01 to 05 connect directly to the base station
 * 011 to 051 relay through 01 to the base station
 * 0125 to 0525 relay through 025, which relays through 05
 * The maximal address is 05555
 */
#define RADIO_ADDRESS	01
/*
 * By default radios are in relay mode, allowing leaf
 * nodes to be added. If this is to be a terminal leaf
 * then set RADIO_RELAY to false
 */
#define RADIO_RELAY	true
/*
 * NETWORK_LOOP_MS is the minimum number of ms between
 * network update.  The actual time is modified by the
 * RADIO_ADDRESS to ensure an offset.
 */
#define NETWORK_LOOP_MS	50

/*
 * MAX_TEMP_SENSORS defines how many sensors are in
 * use.  Currently this can be either 1 or 2.
 * Setting this incorrectly can impact on operation.
 */
#define MAX_TEMP_SENSORS	1
/*
 * SENSOR_LOOP_MS is the delay between checking on
 * (and acting upon) sensor input.  Again this is
 * offset by the RADIO_ADDRESS.
 */
#define SENSOR_LOOP_MS	1995

/*
 * Times are set at UTC, so the TZ_OFFSET is the
 * number of hours to adjust things like the Start/End
 * time values to match the current date/time.
 */
#define TZ_OFFSET 10

/*
 * Each of the HAS_* defines determines what hardware
 * is available.
 *
 * HAS_LED_DISPLAY defines if there is a 4 digit LED
 * display along with up/down buttons for setting
 * configuration options.
 */
#define HAS_LED_DISPLAY 1
/*
 * If using the TinyRTC board, which has an EEPROM
 * on board, you can define this to allow persistance
 * of configuration items.  There should be a fallback
 * of using the Arduino inbuilt EEPROM, however that has
 * not yet been implemented.
 */
#define HAS_EEPROM 1
/*
 * Using the TinyRTC board there is an RTC chip that
 * can be used as a time source.  Setting this enables
 * the RTC.  Note that if the RTC is absent and this is
 * set the unit will not start as it polls for the existence
 * of the RTC prior to going into the main loop.
 */
#define HAS_RTC 1
/*
 * For devices that have a time managed component, setting
 * this enables a second relay that can be controlled by
 * the low_time/high_time configuration items.
 */
#define HAS_TIMED_RELAY 1

/*
 * DEBUG controls what appears on the serial port and how
 * much noise is generated over the radio.  Set this only
 * for testing as it can swamp a large network.
 */
#define DEBUG 0

/*
 * The sentinel used in the EEPROM to determine if we have
 * been configured.  If there are changes to the structure
 * of config memory layout (for instance by changing the
 * MAX_TEMP_SENSORS value) then this should be changed so
 * that the EEPROM contents are invalidated.
 */
#define CONFIGURED     0xeb

/*
 * To reduce code/memory usage you can disable the alarm
 * code in the DallasTemperatureSensor library by setting
 * REQUIRESALARMS to false
 */
#define REQUIRESALARMS	false

// some forward definitions to assist in include ordering
void readConfig(void);
void writeConfig(void);
