#ifndef _Register_h_
#define _Register_h_

#define INVALID_CARD 255

// User flags
#define USER_DENIED (1 << 0)
#define USER_ADMIN  (1 << 1)

// 64 bytes
struct User
{
    char    name[46];
    char    telNr[16];
    uint8_t flags;
    uint8_t __reserved;
}
__attribute__((__packed__));


//
// Find a card in index
//
//  cardId: 8 bytes
//
uint8_t registerFindCard(const uint8_t* cardId);

//
// Wipe card index
//
void registerEraseCards();
void registerEraseCard(uint8_t slot);

uint8_t registerFindFreeSlot();

uint8_t* registerGetBuffer();

void registerWriteCardId(const uint8_t* cardId, uint8_t slot);
void registerWriteUser(const User* user, uint8_t slot);

uint8_t* registerReadCardId(uint8_t slot);
User*    registerReadUser(uint8_t slot);

#endif
