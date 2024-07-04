#include "Position.h"

uint64_t tileBlocksCheck;
uint64_t pinnedPossibleMoves[64];
uint64_t tileAttacked = 0;//Used for faster king move generation in endgame
uint64_t tilePinnedBitmask = 0;
int8_t nullPinnedDir = -1;
bool inCheck = 0;
bool inDoubleCheck = 0;