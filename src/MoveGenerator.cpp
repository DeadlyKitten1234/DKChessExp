#include "MoveGenerator.h"
#include "PrecomputedData.h"

uint64_t tileAttacked;
uint64_t tileDoubleAttacked;
uint64_t tileBlocksCheck;
int8_t tilePinnedDir[64];
int8_t nullPinnedDir = -1;

inline bool tileAttackedBySlidingPiece(const Position& pos, int8_t tile, int8_t ignoreStTile, int8_t addEndTile, int8_t ignoreEnPassantCaptureTile) {
	uint64_t blockers = (pos.m_whiteAllPiecesBitboard | pos.m_blackAllPiecesBitboard);
	blockers ^= (1ULL << ignoreStTile) | (1ULL << ignoreEnPassantCaptureTile);
	blockers |= 1ULL << addEndTile;
	uint64_t enemyDiagPcs = (pos.m_blackToMove ? pos.m_whiteBitboards[BISHOP] | pos.m_whiteBitboards[QUEEN] :
											 	 pos.m_blackBitboards[BISHOP] | pos.m_blackBitboards[QUEEN]);
	if (bishopMovesLookup[tile][getMagicIdx(blockers & bishopRelevantSq[tile], tile, 1)] & enemyDiagPcs) {
		return 1;
	}
	uint64_t enemyStraightPcs = (pos.m_blackToMove ? pos.m_whiteBitboards[ROOK] | pos.m_whiteBitboards[QUEEN] :
													 pos.m_blackBitboards[ROOK] | pos.m_blackBitboards[QUEEN]);
	if (rookMovesLookup[tile][getMagicIdx(blockers & rookRelevantSq[tile], tile, 0)] & enemyStraightPcs) {
		return 1;
	}
	return 0;
}

inline int16_t generateKingMoves(const Position& pos, int16_t* out, int16_t curMoveCnt, bool inCheck) {
	int16_t ans = curMoveCnt;
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
				out[ans++] = createMove(king->pos, king->pos + 2);
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
				out[ans++] = createMove(king->pos, king->pos - 2);

			}
		}
	}
	//Extract moves from bitmask
	while (possibleMoves != 0) {
		int8_t curPos = getLSBPos(possibleMoves);
		out[ans++] = createMove(king->pos, curPos);
		possibleMoves &= possibleMoves - 1;
	}
	return ans - curMoveCnt;
}

inline int16_t extractPawnMovesFromBitmask(int16_t* out, int16_t moveStIdx, uint64_t bitmask, int8_t dirIdx, bool doubleMove, bool blackToMove) {
	int16_t ans = moveStIdx;
	while (bitmask != 0) {
		int8_t pos = getLSBPos(bitmask);
		int8_t stPos = pos - dirAdd[dirIdx] - (doubleMove ? dirAdd[dirIdx] : 0);
		if (tilePinnedDir[stPos] == nullPinnedDir || tilePinnedDir[stPos] == dirIdx || 8 - tilePinnedDir[stPos] == dirIdx) {
			if ((blackToMove && pos < 8) || (!blackToMove && pos > 55)) {
				for (int8_t i = QUEEN; i <= ROOK; i++) {
					out[ans++] = createMove(stPos, pos, PieceType(i));
				}
			} else {
				out[ans++] = createMove(stPos, pos);
			}
		}
		bitmask &= bitmask - 1;
	}
	return ans - moveStIdx;
}

inline int16_t generatePawnMoves(const Position& pos, int16_t* out, int16_t moveStIdx, bool inCheck) {
	int16_t ans = moveStIdx;
	const uint64_t friendlyPawnsBitboard = (pos.m_blackToMove ? pos.m_blackBitboards[PAWN] : pos.m_whiteBitboards[PAWN]);
	const uint64_t emptySq = ~(pos.m_whiteAllPiecesBitboard | pos.m_blackAllPiecesBitboard);
	uint64_t moveB1, moveB2;
	//Regular and double move
	if (pos.m_blackToMove) {
		moveB1 = (friendlyPawnsBitboard >> 8) & emptySq;
		moveB2 = (moveB1 >> 8) & emptySq & rowBitmask[4];
	} else {
		moveB1 = (friendlyPawnsBitboard << 8) & emptySq;
		moveB2 = (moveB1 << 8) & emptySq & rowBitmask[3];
	}
	if (inCheck) {
		moveB1 &= tileBlocksCheck;
		moveB2 &= tileBlocksCheck;
	}
	int8_t dirIdx = dirIdxFromCoordChange[(pos.m_blackToMove ? 0 : 2)][1];
	ans += extractPawnMovesFromBitmask(out, ans, moveB1, dirIdx, 0, pos.m_blackToMove);
	ans += extractPawnMovesFromBitmask(out, ans, moveB2, dirIdx, 1, pos.m_blackToMove);
	//Captures
	uint64_t epBitboard1 = 0;
	uint64_t epBitboard2 = 0;
	if (pos.m_possibleEnPassant != -1) {
		int8_t enPassantablePawnPos = pos.m_possibleEnPassant + (pos.m_blackToMove ? 8 : -8);
		int8_t pawnToEnPassantPos = enPassantablePawnPos + 1;
		int8_t friendlyKingPos = (pos.m_blackToMove ? pos.m_blackPiece[0]->pos : pos.m_whitePiece[0]->pos);
		//Check if after en passant pawn will reveal attack on king
		if (!tileAttackedBySlidingPiece(pos, friendlyKingPos, pawnToEnPassantPos, pos.m_possibleEnPassant, enPassantablePawnPos)) {
			epBitboard1 = (1ULL << pos.m_possibleEnPassant);
		}
		pawnToEnPassantPos -= 2;
		//Check if after en passant pawn will reveal attack on king
		if (!tileAttackedBySlidingPiece(pos, friendlyKingPos, pawnToEnPassantPos, pos.m_possibleEnPassant, enPassantablePawnPos)) {
			epBitboard2 = (1ULL << pos.m_possibleEnPassant);
		}
	}
	if (pos.m_blackToMove) {
		moveB1 = ((friendlyPawnsBitboard & ~colBitmask[0]) >> 9) & (pos.m_whiteAllPiecesBitboard | epBitboard1);
		moveB2 = ((friendlyPawnsBitboard & ~colBitmask[7]) >> 7) & (pos.m_whiteAllPiecesBitboard | epBitboard2);
	} else {
		moveB1 = ((friendlyPawnsBitboard & ~colBitmask[0]) << 7) & (pos.m_blackAllPiecesBitboard | epBitboard1);
		moveB2 = ((friendlyPawnsBitboard & ~colBitmask[7]) << 9) & (pos.m_blackAllPiecesBitboard | epBitboard2);
	}
	if (inCheck) {
		moveB1 &= tileBlocksCheck | epBitboard1;
		moveB2 &= tileBlocksCheck | epBitboard2;
	}
	dirIdx = dirIdxFromCoordChange[(pos.m_blackToMove ? 0 : 2)][0];
	ans += extractPawnMovesFromBitmask(out, ans, moveB1, dirIdx, 0, pos.m_blackToMove);
	dirIdx = dirIdxFromCoordChange[(pos.m_blackToMove ? 0 : 2)][2];
	ans += extractPawnMovesFromBitmask(out, ans, moveB2, dirIdx, 0, pos.m_blackToMove);
	return ans - moveStIdx;
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
	int16_t ans = curMoveCnt;
	while (possibleMoves != 0) {
		int8_t curPos = getLSBPos(possibleMoves);
		out[ans++] = createMove(piece.pos, curPos);
		possibleMoves &= possibleMoves - 1;
	}
	return ans - curMoveCnt;
}

inline int16_t generateKnightMoves(const Position& pos, int16_t* out, int16_t curMoveCnt, Piece& knight, bool inCheck) {
	int16_t ans = curMoveCnt;
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
		out[ans++] = createMove(knight.pos, curPos);
		possibleMoves &= possibleMoves - 1;
	}
	return ans - curMoveCnt;
}

inline void addAttack(uint64_t bitmask) {
	tileDoubleAttacked |= tileAttacked & bitmask;
	tileAttacked |= bitmask;
}

inline void generateBishopAttackTiles(Piece* bishop, Piece* friendlyKing, uint64_t allPiecesBitboard) {
	uint64_t kingBitboard = (1ULL << friendlyKing->pos);
	allPiecesBitboard &= ~(kingBitboard);
	int8_t pos = bishop->pos;
	uint64_t relevantBitmask = bishopRelevantSq[pos] & allPiecesBitboard;
	addAttack(bishopMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 1)]);
	//Set blocks check
	if (bishopMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 1)] & kingBitboard) {
		relevantBitmask = bishopRelevantSq[pos] & (allPiecesBitboard | kingBitboard);
		uint64_t kingRelevantBitmask = bishopRelevantSq[friendlyKing->pos] & allPiecesBitboard;
		//Get bitmask of bishopmoves from bishop and king and say their overlap blocks check
		tileBlocksCheck |= bishopMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 1)] &
			bishopMovesLookup[friendlyKing->pos][getMagicIdx(kingRelevantBitmask, friendlyKing->pos, 1)];
		tileBlocksCheck |= (1ULL << pos);
	}
	//Add pins
	if (bishop->diagMain == friendlyKing->diagMain || bishop->diagScnd == friendlyKing->diagScnd) {
		relevantBitmask = bishopRelevantSq[pos] & (allPiecesBitboard | kingBitboard);
		uint64_t kingRelevantBitmask = bishopRelevantSq[friendlyKing->pos] & allPiecesBitboard;
		//Get bitmask of bishopmoves from bishop and king and see if they overlap on a square with a piece on it
		uint64_t commonMoveBitmask = bishopMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 1)] &
			bishopMovesLookup[friendlyKing->pos][getMagicIdx(kingRelevantBitmask, friendlyKing->pos, 1)];
		//If enemy piece is on tile we dont care, because we won't try to move it anyway
		// && (commonMoveBitmask & (commonMoveBitmask - 1)) == 0 is to check if commonMoveBitmask has only 1 bit
		if (commonMoveBitmask != 0 && (commonMoveBitmask & (commonMoveBitmask - 1)) == 0) {
			tilePinnedDir[getLSBPos(commonMoveBitmask)] = (bishop->diagMain == friendlyKing->diagMain ? 0 : 2);
		}
	}
}

inline void generateRookAttackTiles(Piece* rook, Piece* friendlyKing, uint64_t allPiecesBitboard) {
	uint64_t kingBitboard = (1ULL << friendlyKing->pos);
	allPiecesBitboard &= ~(kingBitboard);
	int8_t pos = rook->pos;
	uint64_t relevantBitmask = rookRelevantSq[pos] & allPiecesBitboard;
	addAttack(rookMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 0)]);
	//Set blocks check
	if ((rookMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 0)] & kingBitboard)  != 0) {
		relevantBitmask = rookRelevantSq[pos] & (allPiecesBitboard | kingBitboard);
		uint64_t kingRelevantBitmask = rookRelevantSq[friendlyKing->pos] & allPiecesBitboard;
		//Get bitmask of rookmoves from rook and king and say their overlap blocks check
		tileBlocksCheck |= rookMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 0)] &
			rookMovesLookup[friendlyKing->pos][getMagicIdx(kingRelevantBitmask, friendlyKing->pos, 0)];
		tileBlocksCheck |= (1ULL << pos);
	}
	//Add pins
	if (rook->x == friendlyKing->x || rook->y == friendlyKing->y) {
		relevantBitmask = rookRelevantSq[pos] & (allPiecesBitboard | kingBitboard);
		uint64_t kingRelevantBitmask = rookRelevantSq[friendlyKing->pos] & allPiecesBitboard;
		//Get bitmask of rookmoves from rook and king and see if they overlap on a square with a piece on it
		uint64_t commonMoveBitmask = rookMovesLookup[pos][getMagicIdx(relevantBitmask, pos, 0)] &
			rookMovesLookup[friendlyKing->pos][getMagicIdx(kingRelevantBitmask, friendlyKing->pos, 0)];
		//If enemy piece is on tile we dont care, because we won't try to move it anyway
		// && (commonMoveBitmask & (commonMoveBitmask - 1)) == 0 is to check if commonMoveBitmask has only 1 bit
		if (commonMoveBitmask != 0 && (commonMoveBitmask & (commonMoveBitmask - 1)) == 0) {
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
