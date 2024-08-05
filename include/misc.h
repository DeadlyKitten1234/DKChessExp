#pragma once
#include <cinttypes>

inline int8_t getX(const int8_t pos) {
	return (pos & 0b111);
};
inline int8_t getY(const int8_t pos) {
	return (pos & 0b111000) >> 3;
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

inline int8_t floorLog2(int num) {
	if (num < 0) {
		return 0;
	}
	int8_t ans = 0;
	if (num >> 16) {
		num >>= 16;
		ans |= 16;
	}
	if (num >> 8) {
		num >>= 8;
		ans |= 8;
	}
	if (num >> 4) {
		num >>= 4;
		ans |= 4;
	}
	if (num >> 2) {
		num >>= 2;
		ans |= 2;
	}
	if (num >> 1) {
		num >>= 1;
		ans++;
	}
	return ans;
}

enum SqrtSetting : bool {
	FAST = 0,
	ACCURATE = 1
};

template<SqrtSetting accurateSetting = FAST>
inline int16_t fastSqrt(int num) {
	//Source: https://en.wikipedia.org/wiki/Fast_inverse_square_root
	//Perform some magic with the bits ang get the result we want
	long i;
	float x2, y;
	const float threehalfs = 1.5F;
	x2 = num * 0.5F;
	y = num;
	i = *(long*)&y;
	i = 0x5f3759df - (i >> 1);
	y = *(float*)&i;
	//Still gives fairly accurate results without the line below
	//(it is at most one off for the numbers up to 1000)
	if (accurateSetting == ACCURATE) {
		y = y * (threehalfs - (x2 * y * y));
	}
	return 1/y;
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
void reverseFenPosition(char* fen);

class RandNumGen {
public:
	RandNumGen();
	RandNumGen(uint64_t seed_);
	~RandNumGen() {}

	uint64_t rand();

private:
	uint64_t seed;
};