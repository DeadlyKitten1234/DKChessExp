#pragma once
#include <cinttypes>
//Everywhere generation refers to the "index" of the current search using the tt
//Higher generation means newer; We use generation to determine how much a given entry is "worth"

enum BoundType : int8_t {
	BoundNULL = 0,
	LowBound = 1,
	HighBound = 2,
	Exact = 3
};

class TTEntry {
public:
	TTEntry() { key1 = key2 = bestMove = eval = depth = genAndType = 0; }
	~TTEntry() {}

	inline void setGen(int8_t gen) { genAndType &= 7; genAndType |= gen << 3; }
	inline void setBoundType(BoundType bType) { genAndType &= 0xFC; genAndType |= bType; }
	inline uint8_t gen() const { return (genAndType & 0xF8) >> 3; }
	inline BoundType boundType() const { return BoundType(genAndType & 3); }
	inline bool sameKey(const uint64_t key_) const {
		return (((key_ >> 16) & 0xFFFF) == key1) && ((key_ & 0xFFFF) == key2);
	}
	inline void init(uint64_t key_, int16_t bestMove_, int16_t eval_, uint8_t depth_, uint8_t gen_, BoundType type_) {
		key1 = (key_ >> 16) & 0xFFFF;
		key2 = key_ & 0xFFFF;
		bestMove = bestMove_;
		eval = eval_;
		depth = depth_;
		genAndType = (gen_ << 3) + type_;
	}
	inline uint16_t calcValue() {
		//This function is used to decide what the worth of an entry is
		//in order to pick the least valuable one and replace it with a new one

		//Here we use the genAndType arrangement, where the generation is 
		//already shifted left 3 => weigh generation 8 times as much as depth
		return (genAndType & 0xF8) + depth + (4 * (boundType() == BoundType::Exact)) + (2 * (boundType() == BoundType::LowBound));
	}

	//malloc allocates 12 bytes for optimisation if a int32_t is used?
	//Howerver we want class to take up 10 bytes, so key is split into 2 * int16_t
	//Note: we don't use the full 64 bit key, because in tt first 19(if tt is 16mb) bits are used
	//as index and here we use last 32 bits for confirmation key. Because zobrist hashing has
	//collisions even with 64 bits, using 51 bits will increase the probability of collision
	//2^13 = 8192 times, but it should still be quite rare
	uint16_t key1;
	uint16_t key2;
	int16_t bestMove;
	int16_t eval;
	uint8_t depth;
	uint8_t genAndType;//First 5 bits - generation; 1 bit for later use maybe?; 2 bits for type
};

class TTEntryChunk {
public:
	TTEntryChunk() {}
	~TTEntryChunk() {
		if (entry != nullptr) {
			delete[] entry;
		}
		if (garbageBytesSoSizeIs32Bytes != nullptr) {
			delete[] garbageBytesSoSizeIs32Bytes;
		}
	}

	TTEntry entry[3];
	char garbageBytesSoSizeIs32Bytes[2];
};

const uint8_t chunkSz = sizeof(TTEntryChunk);

class TranspositionTable {
public:
	TranspositionTable();
	~TranspositionTable() {};

	static_assert(sizeof(TTEntryChunk) == 32, "Size of EntryChunk is not 32 bytes");

	void newGen() {
		gen++;
		if (gen >= 32) {
			gen = 0;
		}
	}
	void setSize(const int sizeMB);//Size must be a power of 2
	void clear();

	inline TTEntry* __stdcall find(const uint64_t key, bool& found) const;

	int chunksCnt;
	int8_t shRVal;
	TTEntryChunk* chunk;
	uint8_t gen;//Must be < 32 to be able to put into Entry
};

inline TTEntry* __stdcall TranspositionTable::find(const uint64_t key, bool& found) const {
	const int idx = key >> shRVal;
	TTEntry* const entriesInChunk = chunk[idx].entry;
	for (int8_t i = 0; i < 3; i++) {
		if (entriesInChunk[i].sameKey(key)) {
			found = 1;
			return &entriesInChunk[i];
		}
	}
	//Even if key hasn't been found, return the least valuable entry to be replaced
	int16_t maxValue = -1;
	TTEntry* ans = nullptr;
	for (int8_t i = 0; i < 3; i++) {
		int16_t val = entriesInChunk[i].calcValue();
		if (entriesInChunk[i].gen() > gen) {
			val -= 0x100;
		}
		if (val > maxValue) {
			maxValue = val;
			ans = &entriesInChunk[i];
		}
	}
	found = 0;
	return ans;
}

extern TranspositionTable tt;