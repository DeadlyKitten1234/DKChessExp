#pragma once
#include<cinttypes>
#include<random>
#include "Position.h"

extern uint64_t hashNums[64][2][6];//[Square][Color][PieceType]
extern uint64_t hashNumsCastling[16];
extern uint64_t hashNumsEp[8];//En passant
extern uint64_t hashNumBlackToMove;

extern uint64_t genRandInt64(std::default_random_engine& RNG, std::uniform_int_distribution<int>& bitDistribution);
extern void populateHashNums();
extern uint64_t getPositionHash(const Position& pos);//Not inline so it is slow; real position hash is updated dynamically