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
	shRVal = 64 - getLSBPos(chunksCnt);
	chunk = new TTEntryChunk[chunksCnt]();
	clear();
}

void TranspositionTable::clear() {
	gen = 0;
	for (int i = 0; i < chunksCnt; i++) {
		std::memset(&chunk[i], 0, chunkSz);
	}
}
