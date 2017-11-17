#ifndef _DECLS_H
#define _DECLS_H
#include "setup.h"
#include "message.h"
#include <SoftTimer.h>


void readConfig(void);
void writeConfig(void);
void configureTemp(void);
void sendMessage(int type, message_t * msg);
void sendTime(void);
#if DEBUG
void printTime();
void printConfig(void);
#endif
void requestConfig(void);
void sendConfigItem(uint32_t item, uint32_t value);
void sendConfig();
#if HAS_RADIO
void networkScanTask(Task *me);
#endif
void sensorScanTask(Task *me);
void setup(void);


#endif
