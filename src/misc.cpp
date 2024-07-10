#include "misc.h"

RandNumGen::RandNumGen() { seed = 0; }

RandNumGen::RandNumGen(uint64_t seed_) { seed = seed_; }

//Code for random number generation taken from stockish
uint64_t RandNumGen::rand() {
	seed ^= seed >> 12, seed ^= seed << 25, seed ^= seed >> 27;
	return seed * 2685821657736338717LL;
}

void reverseFenPosition(char* fen) {
	int i = 0, rowNum = 0;
	char** tempRows = new char*[8]();
	while (fen[i] != ' ') {
		int stVal = i;
		tempRows[rowNum] = new char[8]();
		while (fen[i] != '/' && fen[i] != ' ') {
			//Change pieces color
			if ('a' <= fen[i] && fen[i] <= 'z') {
				fen[i] += 'A' - 'a';
			} else {
				if ('A' <= fen[i] && fen[i] <= 'Z') {
					fen[i] += 'a' - 'A';
				}
			}
			i++;
		}
		i--;
		//Flip board left to right
		for (int j = 0; j <= (i - stVal) / 2; j++) {
			char temp = fen[i - j];
			fen[i - j] = fen[j + stVal];
			fen[j + stVal] = temp;

			tempRows[rowNum][j] = fen[j + stVal];
			tempRows[rowNum][i - j - stVal] = fen[i - j];
		}
		tempRows[rowNum][(i - stVal) / 2] = fen[(i - stVal) / 2 + stVal];
		rowNum++;
		i++;
		if (fen[i] == '/') {
			i++;
		}
	}
	//Flip board top to bottom
	int totalCnt = 0;
	for (int j = 0; j < 8; j++) {
		for (int k = 0; k < 8 && tempRows[7 - j][k]; k++) {
			fen[totalCnt++] = tempRows[7 - j][k];
		}
		fen[totalCnt++] = '/';
	}

	//for (int j = 0; j < 8; j++) {
	//	if (tempRows[j] != nullptr) {
	//		delete[] tempRows[j];
	//	}
	//}
}
