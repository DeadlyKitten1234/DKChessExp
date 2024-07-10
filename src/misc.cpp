#include "misc.h"

RandNumGen::RandNumGen() { seed = 0; }

RandNumGen::RandNumGen(uint64_t seed_) { seed = seed_; }

//Code for random number generation taken from stockish
uint64_t RandNumGen::rand() {
	seed ^= seed >> 12, seed ^= seed << 25, seed ^= seed >> 27;
	return seed * 2685821657736338717LL;
}
