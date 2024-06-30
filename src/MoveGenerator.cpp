#include "MoveGenerator.h"
#include "PrecomputedData.h"

#define debug(x) std::cout << "Line: " << __LINE__ << "; " << #x << " is " << (x) << "\n"

uint64_t tileAttacked;
uint64_t tileDoubleAttacked;
uint64_t tileBlocksCheck;
int8_t tilePinnedDir[64];
int8_t nullPinnedDir = -1;

inline bool tileAttackedBySlidingPiece(const Position& pos, int8_t tile, int8_t ignoreStTile, int8_t addEndTile, int8_t ignoreEnPassantCaptureTile) {
	for (int8_t dirIdx = 0; dirIdx < 9; dirIdx++) {
		if (dirIdx == 4) {
			dirIdx++;
		}
		int8_t curPos = tile;
		while (moveInbounds(getX(curPos), getY(curPos), dirIdx)) {
			curPos += dirAdd[dirIdx];
			if (curPos == addEndTile) {
				break;
			}
			if (pos.m_pieceOnTile[curPos] != nullptr && curPos != ignoreStTile && curPos != ignoreEnPassantCaptureTile) {
				if (pos.m_pieceOnTile[curPos]->black != pos.m_blackToMove) {
					if (pos.m_pieceOnTile[curPos]->getMovesInDir(dirIdx)) {
						return 1;
					}
				}
				break;
			}
		}
	}
	return 0;
}

inline int16_t generateKingMoves(const Position& pos, int16_t* out, int16_t curMoveCnt, bool inCheck) {
	int16_t ans = 0;
	Piece* king = (pos.m_blackToMove ? pos.m_blackPiece[0] : pos.m_whitePiece[0]);
	//Lookup moves and remove ones that have friendly pieces on them
	uint64_t possibleMoves = kingMovesLookup[king->pos] & ~(pos.m_blackToMove ? pos.m_blackAllPiecesBitboard : pos.m_whiteAllPiecesBitboard);
	uint64_t allPiecesBitboard = pos.m_whiteAllPiecesBitboard | pos.m_blackAllPiecesBitboard;
	possibleMoves &= ~tileAttacked;
	//Castling
	bool canCastleKingside = (pos.m_bitmaskCastling & (pos.m_blackToMove ? 2 : 8)) != 0;//blackToMove ? 0010 : 1000
	bool canCastleQueenside = (pos.m_bitmaskCastling & (pos.m_blackToMove ? 1 : 4)) != 0;//blackToMove ? 0001 : 0100
	if (canCastleKingside && !inCheck) {
		uint64_t importansSq = castlingImportantSquares[(pos.m_blackToMove ? 2 : 0)];
		//No pieces in the way
		if (!(importansSq & allPiecesBitboard)) {
			//Can't castle through check
			if (!(tileAttacked & importansSq)) {
				//Directly add move, because not worth |= 1 << newPos to just add it later
				out[curMoveCnt++] = createMove(king->pos, king->pos + 2);
				ans++;
			}
		}
	}
	if (canCastleQueenside && !inCheck) {
		uint64_t importansSq = castlingImportantSquares[(pos.m_blackToMove ? 3 : 1)];
		//No pieces in the way
		if (!(importansSq & allPiecesBitboard)) {
			//Can't castle through check
			//& ~(1 << pos - 3) is here, b1 (or b7 for black) can be attacked and can still castle 
			if (!(tileAttacked & (importansSq & ~(1ULL << (king->pos - 3))))) {
				//Directly add move, because not worth |= 1 << newPos to just add it later
				out[curMoveCnt++] = createMove(king->pos, king->pos - 2);
				ans++;
			}
		}
	}
	//Extract moves from bitmask
	while (possibleMoves != 0) {
		int8_t curPos = getLSBPos(possibleMoves);
		out[curMoveCnt++] = createMove(king->pos, curPos);
		ans++;
		possibleMoves &= possibleMoves - 1;
	}
	return ans;
}

inline int16_t extractPawnMovesFromBitmask(int16_t* out, int16_t moveStIdx, uint64_t bitmask, int8_t enPassantTile, int8_t dirIdx, bool doubleMove, bool blackToMove, bool inCheck) {
	int16_t ans = 0;
	while (bitmask != 0) {
		int8_t pos = getLSBPos(bitmask);
		int8_t stPos = pos - dirAdd[dirIdx] - (doubleMove ? dirAdd[dirIdx] : 0);
		if (tilePinnedDir[stPos] == nullPinnedDir || tilePinnedDir[stPos] == dirIdx || 8 - tilePinnedDir[stPos] == dirIdx) {
			if ((blackToMove && pos < 8) || (!blackToMove && pos > 55)) {
				for (int8_t i = QUEEN; i <= ROOK; i++) {
					out[moveStIdx++] = createMove(stPos, pos, PieceType(i));
					ans++;
				}
			} else {
				out[moveStIdx++] = createMove(stPos, pos);
				ans++;
			}
		}
		bitmask &= bitmask - 1;
	}
	return ans;
}

inline int16_t generatePawnMoves(const Position& pos, int16_t* out, int16_t moveStIdx, bool inCheck) {
	int16_t ans = 0;
	const uint64_t friendlyPawnsBitboard = (pos.m_blackToMove ? pos.m_blackBitboards[PAWN] : pos.m_whiteBitboards[PAWN]);
	int8_t dirIdx = dirIdxFromCoordChange[(pos.m_blackToMove ? 0 : 2)][1];

	//Regular Move
	uint64_t movesBitmask = friendlyPawnsBitboard;
	for (int8_t i = 0; i < 2; i++) {
		//Shift pawn forwards
		if (pos.m_blackToMove) {
			movesBitmask >>= 8;
		} else {
			movesBitmask <<= 8;
		}
		//If double push, only look at pawns on their starting squares
		if (i == 1) {
			//Using values row[3] and row[4], because bitmask has already moved twice
			if (pos.m_blackToMove) {
				movesBitmask &= rowBitmask[4];
			} else {
				movesBitmask &= rowBitmask[3];
			}
		}
		//Remove tiles where there is piece from possible moves
		movesBitmask &= ~(pos.m_whiteAllPiecesBitboard | pos.m_blackAllPiecesBitboard);
		//If in check => &= blocksCheck; Original movesBitmask isn't changed, because we want to use it for double moves too
		uint64_t legalMovesBitmask = movesBitmask;
		if (inCheck) {
			legalMovesBitmask &= tileBlocksCheck;
		}
		//Extract moves from created bitmask
		int16_t res = extractPawnMovesFromBitmask(out, moveStIdx, legalMovesBitmask, pos.m_possibleEnPassant, dirIdx, (i == 1), pos.m_blackToMove, inCheck);
		ans += res;
		moveStIdx += res;
	}
	//Capture
	for (int8_t i = 0; i < 2; i++) {
		movesBitmask = friendlyPawnsBitboard & ~(colBitmask[(i == 0 ? 7 : 0)]);
		//Set dirIdx
		dirIdx = dirIdxFromCoordChange[(pos.m_blackToMove ? 0 : 2)][(i == 0 ? 2 : 0)];
		//Shift pawn forward and change X by 1
		if (pos.m_blackToMove) {
			//Here use '-', because shifting right and to decrease X have to increase shift value, and the same for increasing
			movesBitmask >>= 8 - (i == 0 ? 1 : -1);
		} else {
			movesBitmask <<= 8 + (i == 0 ? 1 : -1);
		}
		//Account for en passant
		uint64_t addEnPassantBitboard = 0;
		if (pos.m_possibleEnPassant != -1) {
			int8_t enPassantablePawnPos = pos.m_possibleEnPassant + (pos.m_blackToMove ? 8 : -8);
			int8_t pawnToEnPassantPos = enPassantablePawnPos - (i == 0 ? 1 : -1);
			int8_t friendlyKingPos = (pos.m_blackToMove ? pos.m_blackPiece[0]->pos : pos.m_whitePiece[0]->pos);
			//Check if after en passant pawn will reveal attack on king
			if (!tileAttackedBySlidingPiece(pos, friendlyKingPos, pawnToEnPassantPos, pos.m_possibleEnPassant, enPassantablePawnPos)) {
				addEnPassantBitboard = (1ULL << pos.m_possibleEnPassant);
			}
		}
		//Remove empty tiles and ones with friendly pieces
		movesBitmask &= (pos.m_blackToMove ? pos.m_whiteAllPiecesBitboard : pos.m_blackAllPiecesBitboard) | addEnPassantBitboard;
		//If in check => &= blocksCheck
		if (inCheck) {
			movesBitmask &= tileBlocksCheck | addEnPassantBitboard;
		}
		//Extract moves from created bitmask
		int16_t res = extractPawnMovesFromBitmask(out, moveStIdx, movesBitmask, pos.m_possibleEnPassant, dirIdx, 0, pos.m_blackToMove, inCheck);
		ans += res;
		moveStIdx += res;
	}
	return ans;
}

inline int16_t generateSlidingPieceMoves(const Position& pos, int16_t* out, int16_t curMoveCnt, Piece& piece, bool inCheck) {
	uint64_t possibleMoves = 0;
	uint64_t allPieces = (pos.m_whiteAllPiecesBitboard | pos.m_blackAllPiecesBitboard) & ~(1ULL << piece.pos);
	if (piece.type == ROOK || piece.type == QUEEN) {
		possibleMoves |= rookMovesLookup[piece.pos][getMagicIdx(allPieces & rookRelevantSq[piece.pos], piece.pos, 0)];
	}
	if (piece.type == BISHOP || piece.type == QUEEN) {
		possibleMoves |= bishopMovesLookup[piece.pos][getMagicIdx(allPieces & bishopRelevantSq[piece.pos], piece.pos, 1)];
	}
	//Remove friendly pieces from possible moves
	possibleMoves &= ~(pos.m_blackToMove ? pos.m_blackAllPiecesBitboard : pos.m_whiteAllPiecesBitboard);
	//If in check => &= tileBlocksCheck
	if (inCheck) {
		possibleMoves &= tileBlocksCheck;
	}
	//If pinned &= pinned possible moves
	if (tilePinnedDir[piece.pos] != nullPinnedDir) {
		if (tilePinnedDir[piece.pos] == 0 || tilePinnedDir[piece.pos] == 8) {
			possibleMoves &= mainDiagBitmask[piece.diagMain];
		}
		if (tilePinnedDir[piece.pos] == 1 || tilePinnedDir[piece.pos] == 7) {
			possibleMoves &= colBitmask[piece.x];
		}
		if (tilePinnedDir[piece.pos] == 2 || tilePinnedDir[piece.pos] == 6) {
			possibleMoves &= scndDiagBitmask[piece.diagScnd];
		}
		if (tilePinnedDir[piece.pos] == 3 || tilePinnedDir[piece.pos] == 5) {
			possibleMoves &= rowBitmask[piece.y];
		}
	}
	//Extract moves from bitmask
	int16_t ans = 0;
	while (possibleMoves != 0) {
		int8_t curPos = getLSBPos(possibleMoves);
		out[curMoveCnt++] = createMove(piece.pos, curPos);
		ans++;
		possibleMoves &= possibleMoves - 1;
	}
	return ans;
}

inline int16_t generateKnightMoves(const Position& pos, int16_t* out, int16_t curMoveCnt, Piece& knight, bool inCheck) {
	int16_t newMovesCnt = 0;
	//Pinned
	if (tilePinnedDir[knight.pos] != nullPinnedDir) {
		return 0;
	}
	//possibleMoves = knightmoves & ~friendlyPieces
	uint64_t possibleMoves = knightMovesLookup[knight.pos] & ~(pos.m_blackToMove ? pos.m_blackAllPiecesBitboard : pos.m_whiteAllPiecesBitboard);
	if (inCheck) {
		possibleMoves &= tileBlocksCheck;
	}
	while (possibleMoves != 0) {
		int8_t curPos = getLSBPos(possibleMoves);
		out[curMoveCnt++] = createMove(knight.pos, curPos);
		newMovesCnt++;
		possibleMoves &= possibleMoves - 1;
	}
	return newMovesCnt;
}

inline void addAttack(uint64_t bitmask) {
	tileDoubleAttacked |= tileAttacked & bitmask;
	tileAttacked |= bitmask;
}

inline void generateBishopAttackTiles(Piece* bishop, Piece* friendlyKing, uint64_t allPiecesBitboard) {
	uint64_t kingBitboard = (1ULL << friendlyKing->pos);
	allPiecesBitboard &= ~(kingBitboard);
	int8_t pos = bishop->pos;
	uint64_t relevantBitmask = bishopRelevantSq[pos] & (allPiecesBitboard & ~(1ULL << pos));
	addAttack(bishopMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 1)]);
	//Set blocks check
	if (bishopMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 1)] & kingBitboard) {
		relevantBitmask = bishopRelevantSq[pos] & ((allPiecesBitboard | kingBitboard) & ~(1ULL << pos));
		uint64_t kingRelevantBitmask = bishopRelevantSq[friendlyKing->pos] & allPiecesBitboard;
		//Get bitmask of bishopmoves from bishop and king and say their overlap blocks check
		tileBlocksCheck |= bishopMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 1)] &
			bishopMovesLookup[friendlyKing->pos][getMagicIdx(kingRelevantBitmask, friendlyKing->pos, 1)];
		tileBlocksCheck |= (1ULL << pos);
	}
	//Add pins
	if (bishop->diagMain == friendlyKing->diagMain || bishop->diagScnd == friendlyKing->diagScnd) {
		relevantBitmask = bishopRelevantSq[pos] & ((allPiecesBitboard | kingBitboard) & ~(1ULL << pos));
		uint64_t kingRelevantBitmask = bishopRelevantSq[friendlyKing->pos] & allPiecesBitboard;
		//Get bitmask of bishopmoves from bishop and king and see if they overlap on a square with a piece on it
		uint64_t commonMoveBitmask = bishopMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 1)] &
			bishopMovesLookup[friendlyKing->pos][getMagicIdx(kingRelevantBitmask, friendlyKing->pos, 1)];
		//If enemy piece is on tile we dont care, because we won't try to move it anyway
		if (commonMoveBitmask != 0) {
			tilePinnedDir[getLSBPos(commonMoveBitmask)] = (bishop->diagMain == friendlyKing->diagMain ? 0 : 2);
		}
	}
}

inline void generateRookAttackTiles(Piece* rook, Piece* friendlyKing, uint64_t allPiecesBitboard) {
	uint64_t kingBitboard = (1ULL << friendlyKing->pos);
	allPiecesBitboard &= ~(kingBitboard);
	int8_t pos = rook->pos;
	uint64_t relevantBitmask = rookRelevantSq[pos] & (allPiecesBitboard & ~(1ULL << pos));
	addAttack(rookMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 0)]);
	//Set blocks check
	if ((rookMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 0)] & kingBitboard)  != 0) {
		relevantBitmask = rookRelevantSq[pos] & ((allPiecesBitboard | kingBitboard) & ~(1ULL << pos));
		uint64_t kingRelevantBitmask = rookRelevantSq[friendlyKing->pos] & allPiecesBitboard;
		//Get bitmask of rookmoves from rook and king and say their overlap blocks check
		tileBlocksCheck |= rookMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 0)] &
			rookMovesLookup[friendlyKing->pos][getMagicIdx(kingRelevantBitmask, friendlyKing->pos, 0)];
		tileBlocksCheck |= (1ULL << pos);
	}
	//Add pins
	if (rook->x == friendlyKing->x || rook->y == friendlyKing->y) {
		relevantBitmask = rookRelevantSq[pos] & ((allPiecesBitboard | kingBitboard) & ~(1ULL << pos));
		uint64_t kingRelevantBitmask = rookRelevantSq[friendlyKing->pos] & allPiecesBitboard;
		//Get bitmask of rookmoves from rook and king and see if they overlap on a square with a piece on it
		uint64_t commonMoveBitmask = rookMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 0)] &
			rookMovesLookup[friendlyKing->pos][getMagicIdx(kingRelevantBitmask, friendlyKing->pos, 0)];
		//If enemy piece is on tile we dont care, because we won't try to move it anyway
		if (commonMoveBitmask != 0) {
			tilePinnedDir[getLSBPos(commonMoveBitmask)] = (rook->x == friendlyKing->x ? 1 : 3);
		}
	}
}

inline void getAttackedTiles(const Position& pos) {
	Piece* friendlyKing = (pos.m_blackToMove ? pos.m_blackPiece[0] : pos.m_whitePiece[0]);
	tileAttacked = 0;
	tileBlocksCheck = 0;
	tileDoubleAttacked = 0;
	for (int8_t i = 0; i < 64; i++) {
		tilePinnedDir[i] = nullPinnedDir;
	}
	int8_t enemyPiecesCnt = (pos.m_blackToMove ? pos.m_whiteTotalPiecesCnt : pos.m_blackTotalPiecesCnt);
	Piece** enemyPieces = (pos.m_blackToMove ? pos.m_whitePiece : pos.m_blackPiece);
	uint64_t kingBitboard = (1ULL << friendlyKing->pos);
	//Pawns
	uint64_t enemyPawnsBitmask = (pos.m_blackToMove ? pos.m_whiteBitboards[PAWN] : pos.m_blackBitboards[PAWN]);
	//If is here, becase we can't << or >> with a negative nubmer :(
	//Pos black to move because pawns are opposite color
	if (pos.m_blackToMove) {
		addAttack((enemyPawnsBitmask & ~(colBitmask[0])) << 7);
		addAttack((enemyPawnsBitmask & ~(colBitmask[7])) << 9);
		tileBlocksCheck |= ((kingBitboard & ~(colBitmask[0])) >> 9) & enemyPawnsBitmask;
		tileBlocksCheck |= ((kingBitboard & ~(colBitmask[7])) >> 7) & enemyPawnsBitmask;
	} else {
		addAttack((enemyPawnsBitmask & ~(colBitmask[0])) >> 9);
		addAttack((enemyPawnsBitmask & ~(colBitmask[7])) >> 7);
		tileBlocksCheck |= ((kingBitboard & ~(colBitmask[0])) << 7) & enemyPawnsBitmask;
		tileBlocksCheck |= ((kingBitboard & ~(colBitmask[7])) << 9) & enemyPawnsBitmask;
	}
	//King
	addAttack(kingMovesLookup[(pos.m_blackToMove ? pos.m_whitePiece[0]->pos : pos.m_blackPiece[0]->pos)]);
	//Knights
	uint64_t knightsBitboard = (pos.m_blackToMove ? pos.m_whiteBitboards[KNIGHT] : pos.m_blackBitboards[KNIGHT]);
	while (knightsBitboard != 0) {
		int8_t knightPos = getLSBPos(knightsBitboard);
		//Add attacks
		addAttack(knightMovesLookup[knightPos]);
		//Add blocks check
		if (knightMovesLookup[knightPos] & kingBitboard) {
			tileBlocksCheck |= (1ULL << knightPos);
		}
		knightsBitboard &= knightsBitboard - 1;
	}
	uint64_t allPiecesBitboard = pos.m_whiteAllPiecesBitboard | pos.m_blackAllPiecesBitboard;
	//Bishops and queen diag
	uint64_t diagBitboard = (pos.m_blackToMove ? pos.m_whiteBitboards[BISHOP] | pos.m_whiteBitboards[QUEEN]: 
							 pos.m_blackBitboards[BISHOP] | pos.m_blackBitboards[QUEEN]);
	while (diagBitboard != 0) {
		int8_t piecePos = getLSBPos(diagBitboard);
		generateBishopAttackTiles(pos.m_pieceOnTile[piecePos], friendlyKing, allPiecesBitboard);
		diagBitboard &= diagBitboard - 1;
	}
	//Rook and queen straight
	uint64_t straightBitboard = (pos.m_blackToMove ? pos.m_whiteBitboards[ROOK] | pos.m_whiteBitboards[QUEEN] :
								 pos.m_blackBitboards[ROOK] | pos.m_blackBitboards[QUEEN]);
	while (straightBitboard != 0) {
		int8_t piecePos = getLSBPos(straightBitboard);
		generateRookAttackTiles(pos.m_pieceOnTile[piecePos], friendlyKing, allPiecesBitboard);
		straightBitboard &= straightBitboard - 1;
	}
}

//Returns size
inline int16_t generateMoves(const Position& pos, int16_t* out, int16_t outStIdx) {
	Piece** friendlyPiece = (pos.m_blackToMove ? pos.m_blackPiece : pos.m_whitePiece);
	getAttackedTiles(pos);
	int16_t movesCnt = outStIdx;
	bool inCheck = (tileAttacked & (pos.m_blackToMove ? pos.m_blackBitboards[KING] : pos.m_whiteBitboards[KING])) != 0;
	movesCnt += generateKingMoves(pos, out, movesCnt, inCheck);
	if (tileDoubleAttacked & (pos.m_blackToMove ? pos.m_blackBitboards[KING] : pos.m_whiteBitboards[KING])) {
		return movesCnt - outStIdx;
	}
	//Pawns
	movesCnt += generatePawnMoves(pos, out, movesCnt, inCheck);
	//Knights
	uint64_t knightsBitboard = (pos.m_blackToMove ? pos.m_blackBitboards[KNIGHT] : pos.m_whiteBitboards[KNIGHT]);
	while (knightsBitboard != 0) {
		int8_t knightPos = getLSBPos(knightsBitboard);
		movesCnt += generateKnightMoves(pos, out, movesCnt, *pos.m_pieceOnTile[knightPos], inCheck);
		knightsBitboard &= knightsBitboard - 1;
	}
	//Sliding pieces
	uint64_t slidingPiecesBitboard = (pos.m_blackToMove ? pos.m_blackBitboards[BISHOP] | pos.m_blackBitboards[ROOK] | pos.m_blackBitboards[QUEEN] :
														  pos.m_whiteBitboards[BISHOP] | pos.m_whiteBitboards[ROOK] | pos.m_whiteBitboards[QUEEN]);
	while (slidingPiecesBitboard != 0) {
		int8_t piecePos = getLSBPos(slidingPiecesBitboard);
		movesCnt += generateSlidingPieceMoves(pos, out, movesCnt, *pos.m_pieceOnTile[piecePos], inCheck);
		slidingPiecesBitboard &= slidingPiecesBitboard - 1;
	}
	return movesCnt - outStIdx;
}
