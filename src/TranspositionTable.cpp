#include "TranspositionTable.h"
#include "misc.h"
#include "Move.h"
#include <cassert>
#include <iostream>

TranspositionTable tt = TranspositionTable();

TranspositionTable::TranspositionTable() {
	chunk = nullptr;
	chunksCnt = 0;
	gen = 0;
	shRVal = 0;
}

void TranspositionTable::setSize(const int sizeMB) {
	assert((sizeMB & (sizeMB - 1)) == 0, "Transposition table size isn't a power of 2");
	assert(sizeMB < 1025, "Too large transposition table");

	if (chunk != nullptr) {
		delete[] chunk;
	}

	chunksCnt = sizeMB * 1024 * 1024 / chunkSz;
	shRVal = 64 - (getLSBPos(sizeMB) + 10 + 10 - getLSBPos(chunkSz));
	chunk = new TTEntryChunk[chunksCnt]();

	clear();
}

void TranspositionTable::clear() {
	for (int i = 0; i < chunksCnt; i++) {
		std::memset(&chunk[i], 0, chunkSz);
	}
}

TTEntry* TranspositionTable::find(uint64_t key, bool& found) const {
	TTEntry* entriesInChunk = chunk[(key >> shRVal)].entry;
	for (int8_t i = 0; i < 3; i++) {
		if (entriesInChunk[i].sameKey(key)) {
			found = 1;
			return &entriesInChunk[i];
		}
	}
	found = 0;
	//Even if key hasn't been found, return the least valuable entry to be replaced
	int16_t maxValue = -1;
	TTEntry* ans = nullptr;
	for (int8_t i = 0; i < 3; i++) {
		int16_t val = entriesInChunk[i].calcValue();
		if (entriesInChunk[i].gen() > gen) {
			val -= 0x100;
		}
		if (val >= maxValue) {
			maxValue = val;
			ans = &entriesInChunk[i];
		}
	}
	return ans;
}
