#include "Seculock.h"
#include "Register.h"
#include "debug.h"

//
// General definitions
//

#define USER_SLOTS     128

// all purpose buffer
static uint8_t buffer[64]; // needs to be bigger than sizeof(User)

//
// Card IDs
//
#define CARD_ID_LEN    8

#define OFFSET_IDS     0x0000
#define CARD_IDS_SIZE  (USER_SLOTS * CARD_ID_LEN)

// # iteration to read all Card IDs
#define CARD_ID_CHUNK_SIZE sizeof(buffer)
#define CARD_ID_CHUNKS     (CARD_IDS_SIZE / CARD_ID_CHUNK_SIZE)

// # card IDs fitting in buffer
#define CARD_IDs_IN_CHUNK  (CARD_ID_CHUNK_SIZE / CARD_ID_LEN)

const uint8_t emptyCardSlot[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

const struct User emptyUser = {
    "", "", 0, 0
};

//
// Users
//
#define OFFSET_USERS   (OFFSET_IDS + CARD_IDS_SIZE)


static bool cardIdEqual(const uint8_t* lhs, const uint8_t* rhs)
{
    const uint8_t* end = lhs + CARD_ID_LEN;
    bool equal = true;

    // for (uint8_t i=0; i < 8; i++) {
    //     if (lhs[i] < 0x10) debugPrint('0');
    //     debugPrint(lhs[i], HEX);
    // }

    // debugPrint(' ');
    // debugPrint('=');
    // debugPrint('=');
    // debugPrint(' ');

    // for (uint8_t i=0; i < 8; i++) {
    //     if (rhs[i] < 0x10) debugPrint('0');
    //     debugPrint(rhs[i], HEX);
    // }
    // debugPrint(' ');
    // debugPrint('?');
    // debugPrint(' ');

    while (lhs != end) {
        equal = (*(lhs++) == *(rhs++)) && equal;
    }

    // debugPrintln(equal);

    return equal;
}

uint8_t registerFindCard(const uint8_t* cardId)
{
    uint8_t slot_found = INVALID_CARD;
    uint8_t curr_slot  = 0;

    // debugPrint(F("Searching for "));
    // for (uint8_t i=0; i < 8; i++) {
    //     if (cardId[i] < 0x10) debugPrint('0');
    //     debugPrint(cardId[i], HEX);
    // }
    // debugPrintln();
    
    for (uint16_t addr = OFFSET_IDS; addr < OFFSET_USERS; addr += CARD_ID_CHUNK_SIZE) {

        // fill buffer
        prom.read(addr, buffer, sizeof(buffer));

        // & compare
        for ( uint8_t id = 0; id < CARD_IDs_IN_CHUNK; id++, curr_slot++) {

            if (cardIdEqual(cardId, buffer + id * CARD_ID_LEN))
                slot_found = curr_slot;
        }
    }

    return slot_found;
}

void registerEraseCards()
{
    memset(buffer, 0xFF, CARD_ID_CHUNK_SIZE);
    for (uint16_t addr = OFFSET_IDS; addr < OFFSET_USERS; addr += CARD_ID_CHUNK_SIZE) {
        prom.write(addr, buffer, CARD_ID_CHUNK_SIZE);
    }
}

void registerEraseCard(uint8_t slot)
{
    uint16_t offset = OFFSET_IDS + CARD_ID_LEN * (uint16_t)slot;
    prom.write(offset, (byte*)emptyCardSlot, CARD_ID_LEN);
    registerWriteUser(&emptyUser, slot);
}

uint8_t registerFindFreeSlot()
{
    uint8_t curr_slot = 0;

    for (uint16_t addr = OFFSET_IDS; addr < OFFSET_USERS; addr += CARD_ID_CHUNK_SIZE) {

        // fill buffer
        prom.read(addr, buffer, sizeof(buffer));

        // & compare
        for ( uint8_t id = 0; id < CARD_IDs_IN_CHUNK; id++, curr_slot++) {

            if (cardIdEqual(emptyCardSlot, buffer + id * CARD_ID_LEN))
                return curr_slot;
        }
    }

    return INVALID_CARD;
}

uint8_t* registerGetBuffer()
{
    return buffer;
}

void registerWriteCardId(const uint8_t* cardId, uint8_t slot)
{
    uint16_t offset = OFFSET_IDS + CARD_ID_LEN * (uint16_t)slot;
    prom.write(offset, (byte*)cardId, CARD_ID_LEN);

    offset = OFFSET_USERS + sizeof(User) * slot;
    memset(buffer, 0, sizeof(buffer));
    prom.write(offset, (byte*)buffer, sizeof(User));
}

void registerWriteUser(const User* user, uint8_t slot)
{
    uint16_t offset = OFFSET_USERS + sizeof(User) * slot;
    prom.write(offset, (byte*)user, sizeof(User));
}

User* registerReadUser(uint8_t slot)
{
    uint16_t offset = OFFSET_USERS + sizeof(User) * slot;
    prom.read(offset, (byte*)buffer, sizeof(User));

    return (User*)buffer;
}
