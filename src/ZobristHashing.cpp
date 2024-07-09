#include "ZobristHashing.h"
#include <random>
uint64_t hashNums[64][2][6];//[Square][Color][PieceType]
uint64_t hashNumsCastling[16];
uint64_t hashNumsEp[8];//En passant
uint64_t hashNumBlackToMove = 0;

uint64_t genRandInt64(std::default_random_engine& RNG, std::uniform_int_distribution<int>& bitDistribution) {
	uint64_t ans = 0;
	for (int i = 0; i < 64; i++) {
		ans += bitDistribution(RNG);
		ans <<= 1;
	}
	return ans;
}

void populateHashNums() {
	//Some magic to generate random numbers
	std::default_random_engine RNG(25146);
	std::uniform_int_distribution<int> bitDistribution(0, 1);
	for (int sq = 0; sq < 64; sq++) {
		for (int color = 0; color < 2; color++) {
			for (int type = 0; type < 6; type++) {
				hashNums[sq][color][type] = genRandInt64(RNG, bitDistribution);
			}
		}
	}
	for (int i = 0; i < 16; i++) {
		hashNumsCastling[i] = genRandInt64(RNG, bitDistribution);
	}
	for (int i = 0; i < 8; i++) {
		hashNumsEp[i] = genRandInt64(RNG, bitDistribution);
	}
	hashNumBlackToMove = genRandInt64(RNG, bitDistribution);
}

uint64_t getPositionHash(const Position& pos) {
	uint64_t ans = 0;
	for (int8_t i = 0; i < pos.m_whiteTotalPiecesCnt; i++) {
		ans ^= hashNums[pos.m_whitePiece[i]->pos][0][pos.m_whitePiece[i]->type];
	}
	for (int8_t i = 0; i < pos.m_blackTotalPiecesCnt; i++) {
		ans ^= hashNums[pos.m_blackPiece[i]->pos][1][pos.m_blackPiece[i]->type];
	}
	ans ^= hashNumsCastling[pos.m_bitmaskCastling];
	if (pos.m_possibleEnPassant != -1) {
		ans ^= hashNumsEp[pos.m_possibleEnPassant & 0b111];
	}
	if (pos.m_blackToMove) {
		ans ^= hashNumBlackToMove;
	}
	return ans;
}
