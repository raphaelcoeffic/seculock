#ifndef _Log_h_
#define _Log_h_

#include <Time.h>

// Events
enum {
    LOG_Unused = 0,
    LOG_Boot,
    LOG_Unlock,
    LOG_Lock
};

// 8 bytes
struct LogEntry
{
    time_t   ts;
    uint8_t  event;
    uint8_t  userId;
    uint16_t __reserved;
}
__attribute__((__packed__));

void logInit();
void logWriteEntry(uint8_t event, uint8_t userId);
bool logReadEntry(uint16_t slot, LogEntry* entry);

#endif
