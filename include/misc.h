#pragma once
#include <cinttypes>
#include <cmath>
#include <xmmintrin.h>//_mm_prefetch

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
	return int8_t(idx);
}
inline int8_t floorLog2(int num) {
	if (num < 0) {
		return 0;
	}
	num |= num >> 1;
	num |= num >> 2;
	num |= num >> 4;
	num |= num >> 8;
	num |= num >> 16;
	return getLSBPos(num + 1) - 1;
}
inline int16_t fastSqrt(int x) {
	//Source: https://en.wikipedia.org/wiki/Fast_inverse_square_root
	//Perform some magic with the bits and get the result we want
	bool neg = 0;
	if (x < 0) {
		x = -x;
		neg = 1;
	}

	long i;
	float x2, y;
	x2 = x * 0.5F;
	y = x;
	i = *(long*)&y;
	i = 0x5f3759df - (i >> 1);
	y = *(float*)&i;
	//Still gives fairly accurate results without the line below
	//(it is at most one off for the numbers up to 1000)
	const float threehalfs = 1.5F;
	y = y * (threehalfs - (x2 * y * y));
	if (neg) {
		y = -y;
	}
	return int16_t(1/y);
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
inline int sigmoid(int x, int maxVal, int midVal, int steepness) {
	return int(double(maxVal) / (1 + std::pow(1.1, midVal - (x * steepness))));
}


enum CacheLevel : int8_t {
	L0 = 0,
	L1 = 1,
	L2 = 2
};
template<CacheLevel cacheLevel = L0>
inline void prefetch(const char* adress) {
	_mm_prefetch(adress, _MM_HINT_T0 + cacheLevel);
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

template<typename T>
class Stack {
public:
	Stack();
	~Stack();
	void init(int maxSz);

	inline T top() const { return entry[endIdx - 1]; }
	inline int size() const { return endIdx; }
	inline bool empty() const { return endIdx == 0; }
	inline void pop() { endIdx--; }
	inline void push(const T& newEntry_) { entry[endIdx++] = newEntry_; };
	inline void clear() { endIdx = 0; };
private:
	int endIdx;
	T* entry;
};

template<typename T>
inline Stack<T>::Stack() {
	entry = nullptr;
	endIdx = 0;
}
template<typename T>
inline Stack<T>::~Stack() {
	if (entry != nullptr) {
		delete[] entry;
	}
}

template<typename T>
inline void Stack<T>::init(int maxSz) {
	if (entry != nullptr) {
		delete[] entry;
	}
	entry = new T[maxSz]();
	endIdx = 0;
}
