#include "Log.h"
#include "Seculock.h"

#include <TimeLib.h>

// Start at 16K boundary
#define LOG_OFFSET       0x04000UL
#define LOG_MAX_ADDR     0x10000UL

// 1KB per block
#define LOG_BLOCKS       ((LOG_MAX_ADDR - LOG_OFFSET) >> 10)

// 64 bytes
struct LogTable {
    uint8_t     version[4];
    uint8_t     blocks[48];
    uint8_t __reserved[12];
}
__attribute__((__packed__));

#define BLOCK_TABLE        (LOG_OFFSET + offsetof(LogTable, blocks))
#define BLOCK_ADDR(block)  (LOG_OFFSET + (uint16_t)(block) * 1024)

#define LOG_VERSION_0 0xFE
#define LOG_VERSION_1 0x01
#define LOG_VERSION_2 0x00
#define LOG_VERSION_3 0x00

// 128 entries per 1K block (8 bytes per entry)
#define BLOCK_FULL 0x80

static uint8_t logtable_head;

void logInit()
{
    uint8_t  buffer[sizeof(LogTable::version)];
    uint16_t addr = LOG_OFFSET;

    prom.read(addr, buffer, sizeof(LogTable::version));

    if (buffer[0] != LOG_VERSION_0 ||
        buffer[1] != LOG_VERSION_1 ||
        buffer[2] != LOG_VERSION_2 ||
        buffer[3] != LOG_VERSION_3) {

        buffer[0] = LOG_VERSION_0;
        buffer[1] = LOG_VERSION_1;
        buffer[2] = LOG_VERSION_2;
        buffer[3] = LOG_VERSION_3;

        // Write version
        prom.write(addr, buffer, sizeof(LogTable::version));
        addr += sizeof(LogTable::version);

        // First block starts after block table
        Serial.print("1st block: ");
        Serial.print(addr, HEX);
        Serial.print('/');
        Serial.println(sizeof(LogTable) / sizeof(LogEntry), DEC);

        prom.write(addr++, sizeof(LogTable) / sizeof(LogEntry));
        logtable_head = 0;

        // Wipe block table
        for (; addr < LOG_OFFSET + sizeof(LogTable); addr++) {
            prom.write(addr, 0x00);
        }
    }
    else {
        addr += sizeof(LogTable::version);
        logtable_head = 0;

        // Search first non-full block
        for (; addr < LOG_OFFSET + sizeof(LogTable); addr++, logtable_head++) {
            uint8_t entries = prom.read(addr);
            if (entries < BLOCK_FULL) {
                break;
            }
        }
    }
}

static uint16_t fetchLogAddr()
{
    Serial.print("BLOCK_TABLE = ");
    Serial.println(BLOCK_TABLE, HEX);

    uint16_t entryInBlock = prom.read(BLOCK_TABLE + (uint16_t)logtable_head);
    Serial.print("entryInBlock = ");
    Serial.println(entryInBlock, HEX);
    
    uint16_t addr = BLOCK_ADDR(logtable_head) + sizeof(LogEntry) * entryInBlock;
    Serial.print("addr = ");
    Serial.println(addr, HEX);
    return addr;
}

static void incLogAddr()
{
    uint16_t entryInBlock = prom.read(BLOCK_TABLE + (uint16_t)logtable_head);

    entryInBlock++;
    prom.write(BLOCK_TABLE + (uint16_t)logtable_head, entryInBlock);

    if (entryInBlock == BLOCK_FULL) {
        logtable_head++;
        entryInBlock = 0;

        if (logtable_head >= LOG_BLOCKS) {
            logtable_head = 0;

            // First block starts after block table
            entryInBlock = sizeof(LogTable) / sizeof(LogEntry);
        }
        prom.write(BLOCK_TABLE + (uint16_t)logtable_head, entryInBlock);
    }
}

void logWriteEntry(uint8_t event, uint8_t userId)
{
    LogEntry entry;
    memset(&entry, 0, sizeof(entry));

    entry.ts     = now();
    entry.event  = event;
    entry.userId = userId;

    prom.write(fetchLogAddr(), (byte*)&entry, sizeof(entry));
    incLogAddr();
}

bool logReadEntry(LogEntry* entry)
{
    return false;
}