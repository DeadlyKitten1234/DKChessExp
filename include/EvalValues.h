#pragma once
#include "Piece.h"
#include <cinttypes>
#include <algorithm>
using std::max;

//Values(except king) taken from Stockfish and rescaled so pawn is 100 points
constexpr int16_t pieceValue[6] = { /*King value: INF*/30000, 1220, 400, 375, 610, 100 };

//Values taken from https://www.chessprogramming.org/Simplified_Evaluation_Function
//IMPORTANT: These values are formated like we see the board; But to the computer, 
//these are flipped; Example: sqBonusMidgame[pt][0] we see as the left value but to the
//computer square 0 means bottom left square; Because the values are symetric l to r we can
//use these values directly when evaluating black and can use [63-sq] when evaluating white
const int8_t sqBonusMidgame[6][64] = {
	//King
	{
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-20,-30,-30,-40,-40,-30,-30,-20,
		-10,-20,-20,-20,-20,-20,-20,-10,
		 20, 20,  0,  0,  0,  0, 20, 20,
		 20, 30, 10,  0,  0, 10, 30, 20
	},
	//Queen
	{
		-20,-10,-10, -5, -5,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5,  5,  5,  5,  0,-10,
		 -5,  0,  5,  5,  5,  5,  0, -5,
		  0,  0,  5,  5,  5,  5,  0, -5,
		-10,  5,  5,  5,  5,  5,  0,-10,
		-10,  0,  5,  0,  0,  0,  0,-10,
		-20,-10,-10, -5, -5,-10,-10,-20
	},
	//Bishop
	{
		-20,-10,-10,-10,-10,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5, 10, 10,  5,  0,-10,
		-10,  5,  5, 10, 10,  5,  5,-10,
		-10,  0, 10, 10, 10, 10,  0,-10,
		-10, 10, 10, 10, 10, 10, 10,-10,
		-10,  5,  0,  0,  0,  0,  5,-10,
		-20,-10,-10,-10,-10,-10,-10,-20
	},
	//Knight
	{
		-50,-40,-30,-30,-30,-30,-40,-50,
		-40,-20,  0,  0,  0,  0,-20,-40,
		-30,  0, 10, 15, 15, 10,  0,-30,
		-30,  5, 15, 20, 20, 15,  5,-30,
		-30,  0, 15, 20, 20, 15,  0,-30,
		-30,  5, 10, 15, 15, 10,  5,-30,
		-40,-20,  0,  5,  5,  0,-20,-40,
		-50,-40,-30,-30,-30,-30,-40,-50
	},
	//Rook
	{
		  0,  0,  0,  0,  0,  0,  0,  0,
		  5, 10, 10, 10, 10, 10, 10,  5,
		 -5,  0,  0,  0,  0,  0,  0, -5,
		 -5,  0,  0,  0,  0,  0,  0, -5,
		 -5,  0,  0,  0,  0,  0,  0, -5,
		 -5,  0,  0,  0,  0,  0,  0, -5,
		 -5,  0,  0,  0,  0,  0,  0, -5,
		  0,  0,  0,  5,  5,  0,  0,  0
	},
	//Pawn
	{
		0,  0,  0,  0,  0,  0,  0,  0,
		50, 50, 50, 50, 50, 50, 50, 50,
		10, 10, 20, 30, 30, 20, 10, 10,
		 5,  5, 10, 25, 25, 10,  5,  5,
		 0,  0,  0, 20, 20,  0,  0,  0,
		 5, -5,-10,  0,  0,-10, -5,  5,
		 5, 10, 10,-20,-20, 10, 10,  5,
		 0,  0,  0,  0,  0,  0,  0,  0
	}
};
const int8_t sqKingBonusEndgame[64] = {
	-50,-40,-30,-20,-20,-30,-40,-50,
	-30,-20,-10,  0,  0,-10,-20,-30,
	-30,-10, 20, 30, 30, 20,-10,-30,
	-30,-10, 30, 40, 40, 30,-10,-30,
	-30,-10, 30, 40, 40, 30,-10,-30,
	-30,-10, 20, 30, 30, 20,-10,-30,
	-30,-30,  0,  0,  0,  0,-30,-30,
	-50,-30,-30,-30,-30,-30,-30,-50
};

const int16_t endgameMaterialStart = pieceValue[ROOK] + pieceValue[QUEEN] + pieceValue[BISHOP];

//Return range [0, 128]
inline int16_t getEndgameWeight(const int16_t friendlyEvalNoPawns) {
	//Implementation of this function taken from Sebastian Lague
	//https://github.com/SebLague/Chess-Coding-Adventure/blob/Chess-V1-Unity/Assets/Scripts/Core/AI/Evaluation.cs
	//Have no idea if he has taken it from somewhere else
	//The only change is to mult result by 128, so don't have to work with float or double

	//Save time
	if (friendlyEvalNoPawns > endgameMaterialStart) {
		return 0;
	}

	//return = 128 * (1 - (pcsValueWithoutPawns / endgameMaterialStart))
	return max(0, ((endgameMaterialStart - friendlyEvalNoPawns) << 7) / endgameMaterialStart);
}
inline int16_t getEndgameWeight(const int16_t friendlyEval, const int8_t friendlyPawnsCnt) {
	return getEndgameWeight(friendlyEval - friendlyPawnsCnt * pieceValue[PAWN]);
}
template<bool black>
inline int8_t getSqBonus(const PieceType pt, const int8_t sq) {
	if constexpr (black) {
		return sqBonusMidgame[pt][sq];
	}
	return sqBonusMidgame[pt][63 - sq];
}

//Could later change to sigmoid function
template<bool black>
inline int8_t getKingSqBonus(int8_t sq, const int16_t endgameWeight = 0) {
	if constexpr (!black) {
		sq = 63 - sq;
	}
	if (endgameWeight == 0) {
		return sqBonusMidgame[KING][sq];
	}
	return (sqBonusMidgame[KING][sq] * (128 - endgameWeight) + sqKingBonusEndgame[sq] * endgameWeight) / 128;
}