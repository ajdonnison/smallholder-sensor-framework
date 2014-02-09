/* Saki Sensor Management library.
 *
 * Author: Adam Donnison <adam@sakienvirotech.com>
 * License: LGPL
 *
 * Things to do:
 *
 */

#include <avr/eeprom.h>
#include "Arduino.h"
#include <SoftwareSerial.h>
#include <stdlib.h>
#include <stdio.h>
#include "Saki.h"

SakiManager * _SakiInstance;
SakiConfig _config;

SakiManager::SakiManager(void)
{
  _init();
}

SakiManager::SakiManager(const char * lid, int ninputs, int noutputs, bool allowRemote)
{
  _init();
  id = lid;
  inputs = ninputs;
  outputs = noutputs;
  remote = allowRemote;
}

void
SakiManager::_init(void) 
{
  _radio = XBee();
  _handlerTableSize = 0;
  _inputTable = NULL;
  _outputTable = NULL;
  _inputs = 0;
  _outputs = 0;
  _handlerTable = NULL;
  _destController = XBeeAddress64(0,0);
  _defaultHandler = NULL;
  registerHandler("TM", &_SakiSetTime);
  registerHandler("ID?", &_SakiGetId);
  registerHandler("CF", &_SakiSetConfig);
  registerHandler("CF?", &_SakiGetConfig);
  _alarm = 0;
  _alarmed = false;
  _clockIncrement = 0;
  _clock = 0;
  _clockUpdated = 0;
  _secondsSinceMidnight = 0;
  configChanged = false;
  _packetTimeout = 200;
  _SakiInstance = this;
}

void
SakiManager::start(Stream & serial) {
  _radio.begin(serial);
}

void
SakiManager::debug(bool flag) {
  _debug = flag;
}

void
SakiManager::send(const char * msg) {
  _send(msg, _destController, 0);
}

void
SakiManager::reply(const char * msg) {
  _send(msg, _destRespondant, _shortRespondant);
}

// Does the heavy lifting
void
SakiManager::check() {
  tick();
  ZBRxResponse rx = ZBRxResponse();
  ModemStatusResponse msr = ModemStatusResponse();
  ZBTxStatusResponse txStatus = ZBTxStatusResponse();

  _radio.readPacket(_packetTimeout);
  if (_radio.getResponse().isAvailable()) {
    if (_radio.getResponse().getApiId() == ZB_RX_RESPONSE) {
      _radio.getResponse().getZBRxResponse(rx);
      handle(&rx);
    }
    else if (_radio.getResponse().getApiId() == MODEM_STATUS_RESPONSE) {
      _radio.getResponse().getModemStatusResponse(msr);
      if (msr.getStatus() == DISASSOCIATED) {
        // Do we want to do something here?
      }
    }
    else if (_radio.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
      _radio.getResponse().getZBTxStatusResponse(txStatus);
      _lastDeliveryStatus = txStatus.getDeliveryStatus();
    }
    else {
      _log("INVALID MESSAGE");
    }
  }
}

void
SakiManager::_log(char * msg, bool newline) {
  if ( ! _debug) {
    return;
  }
  if (newline) {
    Serial.println(msg);
  } else {
    Serial.print(msg);
  }
}

void
SakiManager::_logTokens(char ** tokens) {
  if ( ! _debug) {
    return;
  }
  while (*tokens) {
    Serial.print(*tokens++);
    Serial.print(':');
  }
}

void
SakiManager::handle(ZBRxResponse * rx) {
  const char ** Msg;
  byte * msg = rx->getData();
  byte len = rx->getDataLength(); 
  unsigned char hold[len + 1];
  char buf[65];
  void (*handler)(const char **);
  
  _shortRespondant = rx->getRemoteAddress16();
  _destRespondant = rx->getRemoteAddress64();
  for (int i = 0; i < len; i++) {
    hold[i] = msg[i];
  }
  hold[len] = 0;
  memset((void *)buf, 0, 65);
  sprintf(buf, "%lx %lx", _destRespondant.getMsb(), _destRespondant.getLsb());
  Msg = tokenize((char *)hold, ":");
  _log("Message from ", false);
  _log(buf, false);
  _log(": ", false);
  _logTokens(const_cast<char **>(Msg));
  _log("");

  if ( (handler = _handlerRegistered(Msg[0])) != NULL) {
    handler(Msg);
  }
  else {
    reply("NK");
    _log("Invalid message received");
  }

}

callback_t
SakiManager::_handlerRegistered(const char * key) {
  void (*handler)(const char **);
  for (int i = 0; i < _handlerTableSize; i++) {
    if (!strcmp(key, _handlerTable[i].key)) {
      return _handlerTable[i].method;
    }
  }
  if (_defaultHandler != NULL) {
    return _defaultHandler;
  }
  return NULL;
}

void 
SakiManager::registerDefaultHandler(void (*handler)(const char **)) {
  _defaultHandler = handler;
}

void 
SakiManager::registerHandler(const char * key, void (*handler)(const char **)) {
  // Check if it is already registered, if so replace it.
  // This allows us to register standard handlers that can be overridden
  for (int i = 0; i < _handlerTableSize; i++) {
    if (!strcmp(key, _handlerTable[i].key)) {
      _handlerTable[i].method = handler;
      return; // EARLY RETURN!
    }
  }
  // Add a handler to the table
  
  _handlerTable = (_handler_t *)realloc((void *)_handlerTable, sizeof(_handler_t) * (_handlerTableSize+1));
  _handlerTable[_handlerTableSize].key = key;
  _handlerTable[_handlerTableSize].method = handler;
  _handlerTableSize++;
}

void
SakiManager::_send(const char * msg, XBeeAddress64 & addr, uint16_t shortAddr) {
  uint8_t len = strlen(msg);
  ZBTxRequest tx = ZBTxRequest(addr, (uint8_t *)msg, len);
  if (shortAddr) {
    tx.setAddress16(shortAddr);
  }
  _radio.send(tx);
}

const char ** 
SakiManager::tokenize(char * msg, const char * delim) {
  char * tokens[20];
  char * tkmsg = msg;
  int i = 0;
  char * tk;
  do {
    tk = tokens[i++] = strtok(tkmsg, delim);
    tkmsg = NULL;
  }
  while (tk != NULL);
  tokens[i] = NULL; // End sentinel
  return const_cast<const char **>(tokens);
}

unsigned long
SakiManager::clockTime(void) {
  return _clock;
}

void
SakiManager::tick(void) {
  unsigned long lastUpdated;
  if (_clock == 0) { // No sense updating if it hasn't been set
    return;
  }

  lastUpdated = _clockUpdated;
  _clockUpdated = millis();
  // Handle rollover
  if (_clockUpdated < lastUpdated) {
    lastUpdated = 0;
  }
  _clockIncrement += (_clockUpdated - lastUpdated);
  _clock += _clockIncrement / 1000;
  _clockIncrement %= 1000;
  if (_alarm && (_clock > _alarm)) {
    _alarmed = true;
  }
}

void
SakiManager::setAlarm(unsigned long secs, bool delta) {
  _alarmed = false;
  if (delta) {
    _alarm = _clock + secs;
  } else {
    _alarm = secs;
  }
}

void
SakiManager::clearAlarm(void) {
  _alarmed = false;
  _alarm = 0;
}

/* Just set the clock so that tick() works */
void
SakiManager::startClock(void) {
  _clock = 1;
}

bool
SakiManager::isAlarmed(bool clear) {
  bool alarmed = _alarmed;
  if (alarmed && clear) {
    _alarmed = false;
    _alarm = 0;
  }
  return alarmed;
}

void
SakiManager::setDigitalInput(uint8_t line, bool value) {
  _setIO(true, true, line, value ? 1L : 0L, 0);
}

void
SakiManager::setDigitalOutput(uint8_t line, bool value) {
  _setIO(false, true, line, value ? 1L : 0L, 0);
}

void
SakiManager::setAnalogInput(uint8_t line, long value, uint8_t precision) {
  _setIO(true, false, line, value, precision);
}

void
SakiManager::_setIO(bool isInput, bool isDigital, uint8_t line, long value, uint8_t precision) {
  _io_line_t ** ioTable;
  uint8_t * count;
  int size;
  if (isInput) {
    ioTable = &_inputTable;
    count = &_inputs;
  } else {
    ioTable = &_outputTable;
    count = &_outputs;
  }
  if (line >= *count) {
    *count = line + 1;
    size = sizeof(_io_line_t) * (*count);
    *ioTable = (_io_line_t *)realloc(*ioTable, size);
  }
  (*ioTable)[line].digital = isDigital;
  (*ioTable)[line].value = value;
  (*ioTable)[line].precision = precision;
}

void
SakiManager::report(bool toController) {
  char * buf;
  int size;
  int i;
  size = 10 * (_inputs + _outputs);
  buf = (char *)malloc(size);
  sprintf(buf, "ST:%d:%d", _inputs, _outputs);
  size = strlen(buf);
  for (i = 0; i < _inputs; i++) {
    if (_inputTable[i].digital) {
      sprintf(buf+size, ":%s", _inputTable[i].value ? "Y" : "N");
      size = strlen(buf);
    } else {
      if (_inputTable[i].precision) {
        buf[size++] = ':';
        formatWithPrecision(buf+size, _inputTable[i].value, _inputTable[i].precision);
      } else {
        sprintf(buf+size, ":%ld", _inputTable[i].value);
      }
      size = strlen(buf);
    }
  }
  for (i = 0; i < _outputs; i++) {
    if (_outputTable[i].digital) {
      sprintf(buf+size, ":%s", _outputTable[i].value ? "Y" : "N");
      size = strlen(buf);
    } else {
      if (_outputTable[i].precision) {
        buf[size++] = ':';
        formatWithPrecision(buf+size, _outputTable[i].value, _outputTable[i].precision);
      } else {
        sprintf(buf+size, ":%ld", _outputTable[i].value);
      }
      size = strlen(buf);
    }
  }
  if (toController) {
    send(buf);
  } else {
    reply(buf);
  }
  free(buf);
}

void
SakiManager::formatWithPrecision(char * buf, long value, uint8_t precision) {
  switch (precision) {
    case 1:
      sprintf(buf, "%ld.%d", value / 10, value % 10);
      break;
    case 2:
      sprintf(buf, "%ld.%02d", value / 100, value % 100);
      break;
    case 3:
      sprintf(buf, "%ld.%03d", value / 1000, value % 1000);
      break;
    case 4:
      sprintf(buf, "%ld.%04d", value / 10000, value % 10000);
      break;
  }
}

void
SakiManager::setTime(const char **args) {
  _clock = atol(args[1]);
  _secondsSinceMidnight = atol(args[2]);
}

SakiConfig *
SakiManager::getConfig(void) {
  return &_config;
}

void
_SakiSetTime(const char ** args) {
  _SakiInstance->setTime(args);
}

void
_SakiGetId(const char ** args) {
  char buf[32];
  sprintf(buf, "ID:%s:%d:%d:%s", _SakiInstance->id, _SakiInstance->inputs, _SakiInstance->outputs, _SakiInstance->remote ? "Y" : "N");
  _SakiInstance->reply(buf);
}

void
_SakiSetConfig(const char ** args) {
  *args++;
  const char * key;
  const char * value;
  while (*args) {
    key = *args++;
    if ( ! *args) {
      break;
    }
    value = *args++;
    _config.set(key, atol(value));
  }
  _SakiInstance->configChanged = true;
  _config.save();
}

void
_SakiGetConfig(const char ** args) {
  char * buf;
  SakiConfigItem * item;
  int off;
  int sz;
  _config.start();
  sz = 100;
  buf = (char *)malloc(sz);
  memcpy(buf, "CF", 2);
  off = 2;
  while ((item = _config.next()) != NULL) {
    if ((sz - off) < 10) {
      sz += 100;
      buf = (char *)realloc((char *)buf, sz);
    }
    sprintf(buf+off, ":%s", item->print());
    off = strlen(buf);
  }
  _SakiInstance->reply(buf);
  free(buf);
}

SakiConfigItem::SakiConfigItem()
: value(0L)
{
  key[0] = '\0';
  key[1] = '\0';
}

SakiConfigItem::SakiConfigItem(const char * Key)
: value(0L)
{
  key[0] = Key[0];
  key[1] = Key[1];
}

SakiConfig::SakiConfig()
: _data(NULL),
_dataSize(0),
_offset(0)
{
}

void SakiConfig::set(const char * key , long val)
{
  SakiConfigItem * item;
  if ((item = _get(key)) == NULL) {
    item = _add(key);
  }
  item->value = val;
}

/* Only set if the key doesn't already exist */
void SakiConfig::setDefault(const char *key, long val)
{
  SakiConfigItem * item;
  if ((item = _get(key)) == NULL) {
    item = _add(key);
    item->value = val;
  }
}

void SakiConfig::set(const char * key, bool val)
{
  SakiConfigItem * item;
  if ((item = _get(key)) == NULL) {
    item = _add(key);
  }
  item->value = val ? 1L : 0L;
}

long SakiConfig::get(const char * key)
{
  SakiConfigItem * item = _get(key);
  if (item == NULL) {
    return 0;
  }
  return item->value;
}

bool SakiConfig::getBool(const char * key)
{
  SakiConfigItem * item = _get(key);
  if (item == NULL) {
    return false;
  }
  return item->value != 0;
}

void SakiConfig::print(void)
{
  SakiConfigItem * item;
  start();
  while ((item = next()) != NULL) {
    Serial.println(item->print());
  }
}

SakiConfigItem *
SakiConfig::_add(const char * key) {
  int i = _dataSize++;
  SakiConfigItem newItem(key);
  _data = (SakiConfigItem *)realloc((void *)_data, sizeof(SakiConfigItem) * _dataSize);
  memcpy((void *)(_data+i), (void *)&newItem, sizeof(newItem));
  return _data+i;
}

SakiConfigItem *
SakiConfig::_get(const char * key) {
  for (int i = 0; i < _dataSize; i++) {
    if (_data[i] == key) {
      return & _data[i];
    }
  }
  return NULL;
}

void
SakiConfig::load(void) {
  _cfg_store_t stored;

  eeprom_read_block(&stored, (const void *)0, sizeof(stored));
  if (stored.item_count <= 0) {
    return;
  }

  for (int i = 0; i < stored.item_count; i++) {
    set(stored.items[i].key, stored.items[i].value);
  }
}

void
SakiConfig::save(void) {
  _cfg_store_t stored;
  bool changed = false;
  int cnt=0;
  SakiConfigItem * item;
  start();
  eeprom_read_block(&stored, (const void *)0, sizeof(stored));
  while ((item = next()) != NULL) {
    if (stored.items[cnt].key[0] != item->key[0]
    || stored.items[cnt].key[1] != item->key[1]
    || stored.items[cnt].value != item->value) {
      changed = true;
    }
    stored.items[cnt].key[0] = item->key[0];
    stored.items[cnt].key[1] = item->key[1];
    stored.items[cnt].value = item->value;
    cnt++;
  }
  if (stored.item_count != cnt) {
    changed = true;
  }
  stored.item_count = cnt;
  if (changed) {
    eeprom_write_block(&stored, (void *)0, sizeof(stored));
  }
}

bool
SakiConfigItem::operator==(SakiConfigItem them) {
  return (key[0] == them.key[0] && key[1] == them.key[1]);
}

bool
SakiConfigItem::operator==(const char *them) {
  return (them[0] == key[0] && them[1] == key[1]);
}

void
SakiConfig::start(void) {
  _offset = 0;
}

SakiConfigItem *
SakiConfig::next(void) {
  if (_offset >= _dataSize) {
    return NULL;
  }
  return &_data[_offset++];
}

const char *
SakiConfigItem::print(void) {
  static char buf[16];
  memset(buf, 0, 16);
  sprintf(buf, "%c%c:%ld", key[0], key[1], value);
  return const_cast<const char *>(buf);
}

// vim:ai sw=2 expandtab:
