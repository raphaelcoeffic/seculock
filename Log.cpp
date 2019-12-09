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
static uint8_t logtable_tail;

static void writeVersion(byte* buffer)
{
    buffer[0] = LOG_VERSION_0;
    buffer[1] = LOG_VERSION_1;
    buffer[2] = LOG_VERSION_2;
    buffer[3] = LOG_VERSION_3;

    // Write version
    prom.write(LOG_OFFSET, buffer, 4);
}

void wipeLogTable()
{
    // First block has less entries
    prom.write(BLOCK_TABLE, sizeof(LogTable) / sizeof(LogEntry));
    logtable_head = 0;

    // Wipe block table
    for (uint8_t i = 1; i < LOG_BLOCKS; i++) {
        prom.write(BLOCK_TABLE + i, 0x00);
    }
}

void wipeLogClean()
{
    // Wipe log byte by byte
    for (uint16_t i = LOG_OFFSET; i < LOG_MAX_ADDR; i++) {
        prom.write(i, 0x00);
    }

    logInit();
}

void logInit()
{
    uint8_t  buffer[4];
    uint16_t addr = LOG_OFFSET;

    prom.read(addr, buffer, 4);

    if (buffer[0] != LOG_VERSION_0 ||
        buffer[1] != LOG_VERSION_1 ||
        buffer[2] != LOG_VERSION_2 ||
        buffer[3] != LOG_VERSION_3) {

        writeVersion(buffer);
        addr += 4;

        wipeLogTable();
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

        // Search backwards from there the next non-full block
        uint8_t idx = logtable_head - 1;
        logtable_tail = logtable_head;
        
        for (uint8_t i=0; i<LOG_BLOCKS-1; i++, idx--) {
            if (idx >= LOG_BLOCKS)
                idx = LOG_BLOCKS-1;

            // if non-full block: exit
            if (prom.read(BLOCK_TABLE + (uint16_t)idx) != BLOCK_FULL)
                break;

            // otherwise, set the tail
            logtable_tail = idx;
        }
    }
}

static uint16_t fetchLogAddr()
{
    uint16_t entryInBlock = prom.read(BLOCK_TABLE + (uint16_t)logtable_head);
    uint16_t addr = BLOCK_ADDR(logtable_head) + sizeof(LogEntry) * entryInBlock;
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

bool logReadEntry(uint16_t slot, LogEntry* entry)
{
    uint8_t block = logtable_head;

    // Serial.print("# slot = ");
    // Serial.print(slot, DEC);

    while(1) {
        
        // Serial.print("  block = ");
        // Serial.print(block, DEC);

        uint8_t entries = prom.read(BLOCK_TABLE + (uint16_t)block);
        if (entries > slot) {
            // Serial.print("  read 0x");
            uint16_t addr = BLOCK_ADDR(block)
                + sizeof(LogEntry) * (entries - 1 - slot);
            prom.read(addr, (byte*)entry, sizeof(LogEntry));
            // Serial.println(addr, HEX);
            return true;
        }

        if (block != logtable_tail) {
            slot -= entries;
            block = constrain(block-1, 0, LOG_BLOCKS-1);
        }
        else {
            // Serial.println("  exit");
            // End of table reached
            return false;
        }
    }
}
