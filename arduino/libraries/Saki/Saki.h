/**
 * Saki Sensor Management library.  Provides
 * Software/hardware serial options along with XBee
 * methods and a generalised method handoff for handling
 * different sensor/actuator requirements.
 *
 * Author: Adam Donnison <adam@sakienvirotech.com>
 * License: LGPL
 */
#ifndef _SAKI_H
#define _SAKI_H

#include "Arduino.h"
#include <XBee.h>

#include <stdio.h>

typedef void (*callback_t)(const char **);
typedef struct _handler {
  const char * key;
  callback_t method;
} _handler_t;

typedef struct _io_line {
  bool digital;
  long value;
  uint8_t precision;
} _io_line_t;

typedef struct _cfg_item {
  char key[2];
  long value;
} _cfg_item_t;

// Used to store the configs in eeprom.
// The number of items is limited to fit into the smallest eeprom size
typedef struct _cfg_store {
  int item_count;
  _cfg_item_t items[20];
} _cfg_store_t;

// Config class
class SakiConfigItem {
  public:
    SakiConfigItem();
    SakiConfigItem(const char * key);
    char key[2];
    long value;

    const char * print(void);
    bool operator==(SakiConfigItem);
    bool operator==(const char *);
};

class SakiConfig {
  public:
    SakiConfig();
    long get(const char * key);
    bool getBool(const char * key);

    void set(const char * key, long value);
    void set(const char * key, bool value);
    void setDefault(const char *key, long value);
    void start(void);
    void print(void);
    void load(void);
    void save(void);
    SakiConfigItem * next(void);

  private:
    SakiConfigItem * _get(const char * key);
    SakiConfigItem * _add(const char * key);
    
    SakiConfigItem * _data;
    int _dataSize;
    int _offset;
};

class SakiManager {
  public:
    int inputs;
    int outputs;
    const char * id;
    bool remote;
    bool configChanged;
    
    SakiManager(void);
    SakiManager(const char *, int, int, bool);
    void debug(bool flag);
    void send(const char * msg);
    void reply(const char * msg);
    void check();
    void start(Stream &serial);
    void handle(ZBRxResponse *);
    void registerHandler(const char * key, void (*handler)(const char **));
    void registerDefaultHandler(void (*handler)(const char **));
    unsigned long clockTime(void);
    void tick(void);
    void setAlarm(unsigned long, bool delta = true);
    void clearAlarm(void);
    bool isAlarmed(bool clear = false);
    void startClock(void);
    void setDigitalInput(uint8_t ioLine, bool value);
    void setDigitalOutput(uint8_t ioLine, bool value);
    void setAnalogInput(uint8_t ioLine, long value, uint8_t precision);
    void report(bool toController = false);
    void setTime(const char ** args);
    SakiConfig * getConfig(void);

  private:
    XBee _radio;
    uint16_t _lastDeliveryStatus;
    XBeeAddress64 _destController;
    XBeeAddress64 _destRespondant;
    void (*_defaultHandler)(const char **);
    uint16_t _shortRespondant;
    int _packetTimeout;
    _handler_t * _handlerTable;
    int _handlerTableSize;
    bool _debug;
    bool _alarmed;
    unsigned long _alarm;
    _io_line_t * _inputTable;
    _io_line_t * _outputTable;
    uint8_t _inputs;
    uint8_t _outputs;
    int _rx;
    int _tx;
    int _speed;
    unsigned long _clock;
    unsigned long _clockUpdated;
    unsigned long _clockIncrement;
    unsigned long _secondsSinceMidnight;

    void _log(char * msg, bool newline=true);
    void _logTokens(char ** tokens);
    callback_t _handlerRegistered(const char *);
    void _send(const char * msg, XBeeAddress64 & addr, uint16_t shortAddr = 0);
    const char ** tokenize(char * msg, const char * delim);
    void _init();
    void _setIO(bool, bool, uint8_t, long, uint8_t precision = 0);
    void formatWithPrecision(char * buf, long value, uint8_t precision);
};

// handler functions.
void _SakiSetTime(const char **);
void _SakiGetId(const char ** args);
void _SakiSetConfig(const char ** args);
void _SakiGetConfig(const char ** args);
#endif

// vim:ai sw=2 expandtab:
