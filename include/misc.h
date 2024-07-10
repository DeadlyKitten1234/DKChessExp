#pragma once
#include <cinttypes>


inline int8_t getX(const int8_t pos) {
	return (pos & 7);//7 = 111
};
inline int8_t getY(const int8_t pos) {
	return (pos & 56) >> 3;//56 = 111000
};
inline int8_t getDiagM(const int8_t pos) {
	return getX(pos) - getY(pos) + 7;
}
inline int8_t getDiagS(const int8_t pos) {
	return getX(pos) + getY(pos);
}

inline int8_t getLSBPos(const uint64_t bitmask) {
	unsigned long idx;
	_BitScanForward64(&idx, bitmask);
	return idx;
}

inline int8_t countOnes(uint64_t bitmask) {
	int8_t ans = 0;
	while (bitmask) {
		bitmask &= bitmask - 1;
		ans++;
	}
	return ans;
}
inline uint64_t shift(uint64_t number, int8_t amount) {
	return (amount < 0 ? number >> (-amount) : number << amount);
}

class RandNumGen {
public:
	RandNumGen();
	RandNumGen(uint64_t seed_);

	uint64_t rand();

private:
	uint64_t seed;
};