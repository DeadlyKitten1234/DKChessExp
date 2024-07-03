#include "PrecomputedData.h"
#include "Move.h"

const int8_t dirAdd[9] = { -9, -8, -7, -1, -1000, 1, 7, 8, 9 };
const int8_t dirAddX[9] = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
const int8_t dirAddY[9] = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };
const int8_t dirIdxFromCoordChange[3][3] = { {0, 1, 2}, {3, 4, 5}, {6, 7, 8} };//First y + 1, then x + 1
const int8_t knightMoveAdd[8] = { -17, -15, -10, -6, 6, 10, 15, 17 };
const int8_t knightMoveAddX[8] = { -1, 1, -2, 2, -2, 2, -1, 1 };
const int8_t knightMoveAddY[8] = { -2, -2, -1, -1, 1, 1, 2, 2 };

uint64_t rowBitmask[8];
uint64_t colBitmask[8];
uint64_t mainDiagBitmask[15];
uint64_t scndDiagBitmask[15];
uint64_t betweenBitboard[64][64];

uint64_t knightMovesLookup[64];
uint64_t kingMovesLookup[64];
uint64_t castlingImportantSquares[4];//First white; first kingside
uint64_t rookMovesLookup[64][4096];
uint64_t bishopMovesLookup[64][4096];
uint64_t kingMovesCntLookup[64][256];
uint64_t rookMagic[64] = {
2341872357062221840, 342283536008486914, 6953575416982405760, 648535942822895620, 1188954699714267136, 648519445920090114, 144117387107762440, 72080138874912838,
4613375014324945161, 342344077867487232, 4785349767208960, 27584616573567040, 1407443737249024, 1442840765012119556, 577586686638162433, 1407376242507906,
2314888141657342080, 1161933377054457888, 81065069245368320, 11259548966850566, 568447780520192, 282574555447816, 469504659468388354, 9009398286930180,
36029351071858948, 70369820020736, 9025349790736896, 1134773313479168, 3463285727109316864, 18577374232971312, 90354584220340290, 1443720898266809345,
306315144084848768, 598203216953409, 4504149660016640, 2269675501133832, 9025891036301312, 563087459745808, 117665345954644368, 563500816532481,
36031133552754688, 87961467109378, 9007474686361616, 6053119442884296712, 153685406072832008, 4611967510592946178, 288793467979694084, 1696516407332,
2309326811835662464, 27030549550858816, 288265560809029888, 4613946773248934144, 144396731906262272, 4612530529257390848, 9024864595874816, 1441156283108442624,
72339206730416193, 289675409325365379, 35188667596817, 166633805225199873, 342836539360301106, 4684025096166440961, 1135088898052, 4402379499650
};
uint64_t bishopMagic[64]{
1134765797409857, 2269410799132672, 3751499727707897856, 577592192718077952, 18313740617252865, 4632238777312479361, 1153493255082999944, 2327683304407176,
286079219941632, 18049592849727524, 17609377062952, 864699959998562336, 4419593175072, 437166991573516296, 2256489957830916, 2310980073970467848,
189186644488819232, 5197172125215096964, 3397516766745088, 12384934409871376, 383967061228978568, 1837750268983382304, 18084844580118529, 363141208249470992,
9043070960599314, 626020689304357124, 144203166320238784, 171208254230169856, 282575697362944, 20266473368916096, 1153486654795747328, 844699919455301,
1429502564508034, 4613938934958793858, 363104042819389441, 35734665298432, 1693814842851361, 1353454842454016128, 11260168377534468, 45353763728269824,
290484443775504512, 55451704333770792, 396602640835822080, 5068753469703168, 36029901429547521, 1306611278759855112, 4764810750814327296, 1144600228071458,
3463832167745847332, 649091196328738816, 1190076752496959489, 4901042295031726624, 1489005410208981024, 18032129233453062, 721710705115535360, 637721106252048,
282033609769480, 18014956872081544, 18024299516920976, 150332515332, 9162473728, 35210277159172, 5190416184342227008, 585476756375356432
};
uint64_t kingMagic[64] = {
5855101501189333810, 990239780280561663, 2072004575837708938, 5288147535527637304, 3445936005518012792, 6472055562311849825, 5899530368845380629, 5106563027123904038,
8266644041715574485, 456377350781800041, 1081150063585162972, 8652915783771779881, 6384532431374520622, 3738273838925481438, 3963222786879586973, 7045962337799653451,
9205985199335614523, 6439879869263268157, 6303466740247760474, 7669983384992307608, 7193463893102701684, 7900761322489533231, 6650247015642717360, 4945320577222771858,
5709187259896040156, 2114298147088646326, 2271024830493039857, 7278733073459997470, 5317925793221006300, 4105967111262702313, 8160834053124812714, 2656312252428849376,
4129059892204815932, 2250968176304344922, 7043656085504531415, 789276235858788426, 152858871469010535, 8258541294090261095, 5680524695619254325, 1463711828507440869,
7713937298145360104, 3974209531219892893, 568305693285424017, 7089545935629393992, 7696966856552176831, 1619146579235061824, 4386316943797605142, 9134996325679040915,
3612527672490209677, 8653693374198847491, 8970662146112225734, 3807565598348367189, 6285911486202981036, 7496901295577187670, 7785375979333817003, 2667641094218778637,
4144181758666235732, 2982353465979067750, 7955180230553125932, 4278835138319694297, 1563953340875155637, 5543395960080718445, 2567964842475658123, 4206453166288341797 
};
int8_t rookRelevantSqCnt[64];
int8_t bishopRelevantSqCnt[64];
uint64_t rookRelevantSq[64];
uint64_t bishopRelevantSq[64];

void initData() {
	populateGlobalDirectionBitmasks();
	populateMovementBitmasks();
	populateMovesLookup();
	populateKingMovesCnt();
	populateBetweemBitboards();
}

void populateMovementBitmasks() {
	for (int i = 0; i < 64; i++) {
		uint64_t num = 0;
		int numBits = 0;
		//Rook
		for (int j = 0; j < 64; j++) {
			if (((getX(j) == 0 || getX(j) == 7) && getX(j) != getX(i)) || ((getY(j) == 0 || getY(j) == 7) && getY(j) != getY(i))) {
				continue;
			}
			if (((getX(j) == getX(i)) ^ (getY(j) == getY(i))) != 0) {
				num |= 1ULL << j;
				numBits++;
			}
		}
		rookRelevantSq[i] = num;
		rookRelevantSqCnt[i] = numBits;
		//Bishop
		num = 0;
		numBits = 0;
		for (int j = 0; j < 64; j++) {
			if (getX(j) == 0 || getX(j) == 7 || getY(j) == 0 || getY(j) == 7) {
				continue;
			}
			if (((getDiagM(j) == getDiagM(i)) ^ (getDiagS(j) == getDiagS(i))) != 0) {
				num |= 1ULL << j;
				numBits++;
			}
		}
		bishopRelevantSq[i] = num;
		bishopRelevantSqCnt[i] = numBits;
		//Knight
		for (int j = 0; j < 8; j++) {
			if (knightMoveInbounds(getX(i), getY(i), j)) {
				knightMovesLookup[i] |= (1ULL << (i + knightMoveAdd[j]));
			}
		}
		//King
		for (int j = 0; j < 9; j++) {
			if (j == 4) {
				j++;
			}
			if (moveInbounds(getX(i), getY(i), j)) {
				kingMovesLookup[i] |= 1ULL << (i + dirAdd[j]);
			}
		}
	}
}

void populateBetweemBitboards() {
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < 64; j++) {
			if (i == j) {
				continue;
			}
			uint64_t blockers = (1ULL << i) | (1ULL << j);
			if (getX(i) == getX(j) || getY(i) == getY(j)) {
				betweenBitboard[i][j] = attacks<ROOK>(i, blockers) & attacks<ROOK>(j, blockers);
			}
			if (getDiagM(i) == getDiagM(j) || getDiagS(i) == getDiagS(j)) {
				betweenBitboard[i][j] = attacks<BISHOP>(i, blockers) & attacks<BISHOP>(j, blockers);
			}
		}
	}
}

void populateMovesLookup() {
	for (int i = 0; i < 64; i++) {
		//Rook
		for (int j = 0; j < (1ULL << rookRelevantSqCnt[i]); j++) {
			uint64_t blockBitmask = shiftIndexToABlockersBitmask(j, rookRelevantSq[i]);
			uint64_t attBitmask = calcRookAtt(i, blockBitmask);
			rookMovesLookup[i][getMagicIdx(blockBitmask, i, 0)] = attBitmask;
		}
		//Bishop
		for (int j = 0; j < (1ULL << bishopRelevantSqCnt[i]); j++) {
			uint64_t blockBitmask = shiftIndexToABlockersBitmask(j, bishopRelevantSq[i]);
			uint64_t attBitmask = calcBishopAtt(i, blockBitmask);
			bishopMovesLookup[i][getMagicIdx(blockBitmask, i, 1)] = attBitmask;
		}
	}
}

void populateGlobalDirectionBitmasks() {
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			colBitmask[i] |= (1ULL << (i + 8 * j));
			rowBitmask[i] |= (1ULL << (8 * i + j));
		}
	}
	for (int i = 0; i < 15; i++) {
		for (int j = 0; j < 64; j++) {
			if (getDiagM(j) == i) {
				mainDiagBitmask[i] |= (1ULL << j);
			}
			if (getDiagS(j) == i) {
				scndDiagBitmask[i] |= (1ULL << j);
			}
		}
	}
	//Important castling tiles
	castlingImportantSquares[0] = 96;//01100000; board looks with reversed x
	castlingImportantSquares[1] = 14;//00001110; board looks with reversed x
	castlingImportantSquares[2] = castlingImportantSquares[0] << (7 * 8);//Move white castling 7 rows up
	castlingImportantSquares[3] = castlingImportantSquares[1] << (7 * 8);//Move white castling 7 rows up
}

void populateKingMovesCnt() {
	for (int i = 0; i < 64; i++) {
		int numPos = 1ULL << countOnes(kingMovesLookup[i]);
		for (int j = 0; j < numPos; j++) {
			kingMovesCntLookup[i][(shiftIndexToABlockersBitmask(j, kingMovesLookup[i]) * kingMagic[i]) >> (64 - 8)] = countOnes(kingMovesLookup[i]) - countOnes(j);
		}
	}
}

uint64_t calcRookAtt(const int8_t stSquare, const uint64_t blockers) {
	uint64_t ans = 0;
	int x = getX(stSquare) - 1, y = getY(stSquare);
	while (0 <= x) {
		ans |= (1ULL << (x + 8 * y));
		if ((blockers & (1ULL << (x + 8 * y))) != 0) {
			break;
		}
		x--;
	}
	x = getX(stSquare) + 1;
	while (x <= 7) {
		ans |= (1ULL << (x + 8 * y));
		if ((blockers & (1ULL << (x + 8 * y))) != 0) {
			break;
		}
		x++;
	}
	x = getX(stSquare);
	y = getY(stSquare) - 1;
	while (0 <= y) {
		ans |= (1ULL << (x + 8 * y));
		if ((blockers & (1ULL << (x + 8 * y))) != 0) {
			break;
		}
		y--;
	}
	y = getY(stSquare) + 1;
	while (y <= 7) {
		ans |= (1ULL << (x + 8 * y));
		if ((blockers & (1ULL << (x + 8 * y))) != 0) {
			break;
		}
		y++;
	}
	return ans;
}

uint64_t calcBishopAtt(const int8_t stSquare, const uint64_t blockers) {
	uint64_t ans = 0;
	int x = getX(stSquare) - 1, y = getY(stSquare) - 1;
	while (0 <= x && 0 <= y) {
		ans |= (1ULL << (x + 8 * y));
		if ((blockers & (1ULL << (x + 8 * y))) != 0) {
			break;
		}
		x--;
		y--;
	}
	x = getX(stSquare) + 1;
	y = getY(stSquare) - 1;
	while (x <= 7 && 0 <= y) {
		ans |= (1ULL << (x + 8 * y));
		if ((blockers & (1ULL << (x + 8 * y))) != 0) {
			break;
		}
		x++;
		y--;
	}
	x = getX(stSquare) - 1;
	y = getY(stSquare) + 1;
	while (0 <= x && y <= 7) {
		ans |= (1ULL << (x + 8 * y));
		if ((blockers & (1ULL << (x + 8 * y))) != 0) {
			break;
		}
		x--;
		y++;
	}
	x = getX(stSquare) + 1;
	y = getY(stSquare) + 1;
	while (x <= 7 && y <= 7) {
		ans |= (1ULL << (x + 8 * y));
		if ((blockers & (1ULL << (x + 8 * y))) != 0) {
			break;
		}
		x++;
		y++;
	}
	return ans;
}

uint64_t shiftIndexToABlockersBitmask(const int idx, const uint64_t relevantSq) {
	uint64_t ans = 0;
	int squareIdx[12]{ 0 };
	int bits = 0;
	for (int i = 0; i < 64; i++) {
		if ((relevantSq & (1ULL << i)) != 0) {
			squareIdx[bits++] = i;
		}
	}
	for (int j = 0; j < bits; j++) {
		if ((idx & (1 << j)) != 0) {
			ans |= (1ULL << squareIdx[j]);
		}
	}
	return ans;
}

inline int getMagicIdx(const uint64_t bitmask, const int sq, const bool bishop) {
	return (bishop ? ((bitmask * bishopMagic[sq]) >> (64 - bishopRelevantSqCnt[sq])) :
					 ((bitmask * rookMagic[sq]) >> (64 - rookRelevantSqCnt[sq])));
}
