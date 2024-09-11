#include "Position.h"

const int16_t MAX_LEGAL_MOVES = 255;
uint64_t tileBlocksCheck;
uint64_t pinnedPossibleMoves[64];
int8_t piecePinningSq[64];
uint64_t pinnersBB = 0;
int8_t pinnerOfSq[64];
uint64_t tileAttacked = 0;//Used for faster king move generation in endgame
uint64_t tilePinnedBitmask = 0;
bool inCheck = 0;
bool inDoubleCheck = 0;
bool calcChecks = 0;
uint64_t pawnAtt = 0;
uint64_t checks[6] = { 0, 0, 0, 0, 0, 0 };