#include "Position.h"
#include "PrecomputedData.h"
#include "ZobristHashing.h"
using std::swap;
using std::abs;
const char* Position::m_startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

Position::Position() {
	m_blackToMove = 0;
	m_bitmaskCastling = 0;
	m_possibleEnPassant = -1;
	m_legalMoves = new int16_t[MAX_LEGAL_MOVES]();
	m_whitePiece = nullptr;
	m_blackPiece = nullptr;
	m_whitePiecesCnt = nullptr;
	m_blackPiecesCnt = nullptr;
	m_whiteTotalPiecesCnt = 0;
	m_blackTotalPiecesCnt = 0;
	m_pieceOnTile = nullptr;
	m_whiteAllPiecesBitboard = 0;
	m_blackAllPiecesBitboard = 0;
	m_whiteBitboards = nullptr;
	m_blackBitboards = nullptr;
	m_legalMovesCnt = 0;
	friendlyInCheck = 0;
	m_whitePiecesEval = 0;
	m_blackPiecesEval = 0;
	zHash = 0;
	enemyPawnAttacksBitmask = 0;
	m_whiteSqBonusEval = 0;
	m_blackSqBonusEval = 0;
	whiteEndgameWeight = -1;
	blackEndgameWeight = -1;
}

Position::~Position() {
	deleteData();
}

void Position::readFEN(const char* fen) {
	deleteData();
	m_pieceOnTile = new Piece * [64]();
	for (int i = 0; i < 64; i++) {
		m_pieceOnTile[i] = nullptr;
	}
	m_whitePiecesCnt = new int8_t[6]();
	m_blackPiecesCnt = new int8_t[6]();
	m_whiteBitboards = new uint64_t[6]();
	m_blackBitboards = new uint64_t[6]();
	m_legalMoves = new int16_t[MAX_LEGAL_MOVES]();
	m_possibleEnPassant = -1;
	m_whiteAllPiecesBitboard = 0;
	m_blackAllPiecesBitboard = 0;
	m_whiteTotalPiecesCnt = 0;
	m_blackTotalPiecesCnt = 0;
	m_bitmaskCastling = 0;
	m_blackToMove = 0;
	m_whitePiecesEval = 0;
	m_blackPiecesEval = 0;
	whiteEndgameWeight = -1;
	blackEndgameWeight = -1;
	int8_t xPos = 0;
	int8_t yPos = 7;
	int i = 0;
	for (i = 0; fen[i]; i++) {
		if (fen[i] == ' ') {
			break;
		}
		if ('A' <= fen[i] && fen[i] <= 'Z') {
			m_whiteTotalPiecesCnt++;
		}
		if ('a' <= fen[i] && fen[i] <= 'z') {
			m_blackTotalPiecesCnt++;
		}
	}
	m_whitePiece = new Piece * [m_whiteTotalPiecesCnt]();
	m_blackPiece = new Piece * [m_blackTotalPiecesCnt]();
	m_whiteTotalPiecesCnt = 0;
	m_blackTotalPiecesCnt = 0;
	i = 0;
	for (i = 0; fen[i]; i++) {
		if (fen[i] == ' ') {
			i++;
			break;
		}
		if (fen[i] == '/') {
			xPos = 0;
			yPos--;
			continue;
		}
		if ('0' <= fen[i] && fen[i] <= '9') {
			xPos += fen[i] - '0';
			continue;
		}
		if (fen[i] <= 'Z') {
			PieceType type = getTypeFromChar(fen[i]);
			m_whitePiece[m_whiteTotalPiecesCnt] = new Piece(xPos, yPos, type, 0);
			m_pieceOnTile[xPos + 8 * yPos] = m_whitePiece[m_whiteTotalPiecesCnt];
			//If king => swap with first so king is always with idx 0
			if (type == PieceType::KING) {
				swap(m_whitePiece[0], m_whitePiece[m_whiteTotalPiecesCnt]);
			} else {
				//Add to eval
				m_whitePiecesEval += pieceValue[type];
			}
			//Increase count
			m_whiteTotalPiecesCnt++;
			m_whitePiecesCnt[type]++;
			//Bitboards
			m_whiteAllPiecesBitboard |= (1ULL << (xPos + 8 * yPos));
			m_whiteBitboards[type] |= (1ULL << (xPos + 8 * yPos));
		} else {
			PieceType type = getTypeFromChar(fen[i]);
			m_blackPiece[m_blackTotalPiecesCnt] = new Piece(xPos, yPos, type, 1);
			m_pieceOnTile[xPos + 8 * yPos] = m_blackPiece[m_blackTotalPiecesCnt];
			//If king => swap with first so king is always with idx 0
			if (type == PieceType::KING) {
				swap(m_blackPiece[0], m_blackPiece[m_blackTotalPiecesCnt]);
			}
			else {
				//Add to eval
				m_blackPiecesEval += pieceValue[type];
			}
			//Increase count
			m_blackTotalPiecesCnt++;
			m_blackPiecesCnt[type]++;
			//Bitboards
			m_blackAllPiecesBitboard |= (1ULL << (xPos + 8 * yPos));
			m_blackBitboards[type] |= (1ULL << (xPos + 8 * yPos));
		}
		xPos++;
	}
	if (fen[i] == 'b') {
		m_blackToMove = 1;
	} else {
		m_blackToMove = 0;
	}
	i += 2;
	for (; fen[i]; i++) {
		if (fen[i] == ' ') {
			i++;
			break;
		}
		if (fen[i] == 'K' && m_whitePiece[0]->pos == 4 && m_pieceOnTile[7] != nullptr) {
			if (m_pieceOnTile[7]->type == ROOK && m_pieceOnTile[7]->black == 0) {
				m_bitmaskCastling |= 8;
			}
		}
		if (fen[i] == 'Q' && m_whitePiece[0]->pos == 4 && m_pieceOnTile[0] != nullptr) {
			if (m_pieceOnTile[0]->type == ROOK && m_pieceOnTile[0]->black == 0) {
				m_bitmaskCastling |= 4;
			}
		}
		if (fen[i] == 'k' && m_blackPiece[0]->pos == 60 && m_pieceOnTile[63] != nullptr) {
			if (m_pieceOnTile[63]->type == ROOK && m_pieceOnTile[63]->black == 1) {
				m_bitmaskCastling |= 2;
			}
		}
		if (fen[i] == 'q' && m_blackPiece[0]->pos == 60 && m_pieceOnTile[56] != nullptr) {
			if (m_pieceOnTile[56]->type == ROOK && m_pieceOnTile[56]->black == 1) {
				m_bitmaskCastling |= 1;
			}
		}
	}
	if (!(fen[i] == ' ' || fen[i] == '-')) {
		m_possibleEnPassant = -1;
	}
	for (i = i; fen[i]; i++) {
		if (fen[i] == ' ') {
			i++;
			break;
		}
		if ('a' <= fen[i] && fen[i] <= 'h') {
			m_possibleEnPassant |= (fen[i] - 'a');
		}
		if ('1' <= fen[i] && fen[i] <= '8') {
			m_possibleEnPassant |= (fen[i] - '1') << 3;
		}
		if (m_possibleEnPassant != -1) {
			if (getY(m_possibleEnPassant) != 2 && getY(m_possibleEnPassant) != 5) {
				m_possibleEnPassant = -1;
			}

			if (m_pieceOnTile[m_possibleEnPassant + (m_blackToMove ? 8 : -8)] == nullptr ||
				m_pieceOnTile[m_possibleEnPassant + (m_blackToMove ? 8 : -8)]->type != PAWN) {
				//Invalid en passant
				m_possibleEnPassant = -1;
			}
		}
	}
	//For now don't read moves since pawn push and total moves

	//Set indices of pieces; Do it here, because doing it dynamically 
	//would make it a mess (plus its more understandable)
	for (int8_t i = 0; i < m_whiteTotalPiecesCnt; i++) {
		m_whitePiece[i]->idx = i;
	}
	for (int8_t i = 0; i < m_blackTotalPiecesCnt; i++) {
		m_blackPiece[i]->idx = i;
	}
	whiteEndgameWeight = getEndgameWeight(m_whitePiecesEval, m_whitePiecesCnt[PAWN]);
	blackEndgameWeight = getEndgameWeight(m_blackPiecesEval, m_blackPiecesCnt[PAWN]);
	//Set bonuses for sq; Note: evaluate king as if it isn't endgame, because in eval will change it
	for (int8_t i = 0; i < m_whiteTotalPiecesCnt; i++) {
		m_whiteSqBonusEval += getSqBonus<0>(m_whitePiece[i]->type, m_whitePiece[i]->pos);
	}
	for (int8_t i = 0; i < m_blackTotalPiecesCnt; i++) {
		m_blackSqBonusEval += getSqBonus<1>(m_blackPiece[i]->type, m_blackPiece[i]->pos);
	}
	//Set zobrist hash; Do it here for the same reason as setting indices
	zHash = getPositionHash(*this);
	//Set legal moves
	updateLegalMoves<0>(this->m_legalMoves, true);
}

uint64_t Position::getZHashIfMoveMade(const int16_t move) const {
	const int8_t stTile = getStartPos(move), endTile = getEndPos(move);
	const PieceType pt = m_pieceOnTile[stTile]->type;
	//Moving always changes player on turn
	uint64_t ans = zHash ^ hashNumBlackToMove;
	ans ^= hashNums[stTile][m_blackToMove][pt];
	ans ^= hashNums[endTile][m_blackToMove][pt];
	int8_t captureTile = endTile;
	//Handle en passant
	if (pt == PAWN && endTile == m_possibleEnPassant) {
		captureTile = endTile + (m_blackToMove ? 8 : -8);
	}
	//Handle captures
	if (m_pieceOnTile[captureTile] != nullptr) {
		ans ^= hashNums[captureTile][!m_blackToMove][m_pieceOnTile[captureTile]->type];
	}
	//Handle castling
	if (pt == KING && abs(endTile - stTile) == 2) {
		const int8_t rookEndPos = endTile + (stTile > endTile ? 1 : -1);
		const int8_t rookPos = stTile + (stTile > endTile ? -4 : 3);
		ans ^= hashNums[rookPos][m_blackToMove][ROOK];
		ans ^= hashNums[rookEndPos][m_blackToMove][ROOK];
	}
	//Handle castling hash
	ans ^= hashNumsCastling[m_bitmaskCastling];
	int8_t newCastleRights = m_bitmaskCastling;
	if (pt == KING) {
		if (m_blackToMove) {
			newCastleRights &= ~0b0011;
		} else {
			newCastleRights &= ~0b1100;
		}
	}
	if (pt == ROOK) {
		if (stTile == 0) { newCastleRights &= ~4; }//~0100
		if (stTile == 7) { newCastleRights &= ~8; }//~1000
		if (stTile == 56) { newCastleRights &= ~1; }//~0001
		if (stTile == 63) { newCastleRights &= ~2; }//~0010
	}
	if (endTile == 0) { newCastleRights &= ~4; }//~0100
	if (endTile == 7) { newCastleRights &= ~8; }//~1000
	if (endTile == 56) { newCastleRights &= ~1; }//~0001
	if (endTile == 63) { newCastleRights &= ~2; }//~0010
	ans ^= hashNumsCastling[newCastleRights];
	//Handle ep hash
	if (m_possibleEnPassant != -1) {
		ans ^= hashNumsEp[getX(m_possibleEnPassant)];
	}
	if (pt == PAWN && abs(stTile - endTile) == 16) {
		ans ^= hashNumsEp[getX(stTile)];
	}
	return ans;
}

uint64_t Position::getZHashNoEp() const {
	return zHash ^ (m_possibleEnPassant == -1 ? 0 : hashNumsEp[getX(m_possibleEnPassant)]);
}

int8_t Position::makeMove(int16_t move) {
	int8_t stTile = getStartPos(move), endTile = getEndPos(move);

	//Update bitboards, bonuses ans zobrist hash
	if (m_blackToMove) { updateDynamicVars<1>(m_pieceOnTile[stTile]->type, stTile, endTile); }
	else { updateDynamicVars<0>(m_pieceOnTile[stTile]->type, stTile, endTile); }

	zHash ^= hashNumBlackToMove;
	//If castle
	if (m_pieceOnTile[stTile]->type == PieceType::KING && abs(stTile - endTile) == 2) {
		const int8_t rookEndPos = endTile + (stTile > endTile ? 1 : -1);
		int8_t rookPos = stTile + (stTile > endTile ? -4 : 3);
		m_pieceOnTile[rookPos]->setPos(rookEndPos);
		m_pieceOnTile[rookEndPos] = m_pieceOnTile[rookPos];
		m_pieceOnTile[rookPos] = nullptr;
		//Update bb, bonuses and zHash
		if (m_blackToMove) { updateDynamicVars<1>(PieceType::ROOK, rookPos, rookEndPos); }
		else { updateDynamicVars<0>(PieceType::ROOK, rookPos, rookEndPos); }
	}
	//Remove castle rights if moved king or rook
	int8_t begginingCastleRights = m_bitmaskCastling;
	if (m_pieceOnTile[stTile]->type == PieceType::KING) {
		if (m_blackToMove) {
			m_bitmaskCastling &= ~3;//~0011
		} else {
			m_bitmaskCastling &= ~12;//~1100
		}
	}
	if (m_pieceOnTile[stTile]->type == PieceType::ROOK) {
		if (stTile == 0) { m_bitmaskCastling &= ~4; }//~0100
		if (stTile == 7) { m_bitmaskCastling &= ~8; }//~1000
		if (stTile == 56) { m_bitmaskCastling &= ~1; }//~0001
		if (stTile == 63) { m_bitmaskCastling &= ~2; }//~0010
	}
	//Number below is the four corners (where the rooks are)
	//This *might* make it a *little* faster
	if ((1ULL << endTile) & 0x8100000000000081ULL) {
		if (endTile == 0) { m_bitmaskCastling &= ~4; }//~0100
		if (endTile == 7) { m_bitmaskCastling &= ~8; }//~1000
		if (endTile == 56) { m_bitmaskCastling &= ~1; }//~0001
		if (endTile == 63) { m_bitmaskCastling &= ~2; }//~0010
	}
	//Update zobrist hash for castling
	zHash ^= hashNumsCastling[begginingCastleRights];
	zHash ^= hashNumsCastling[m_bitmaskCastling];
	//Promote
	int8_t promotedPawnIdx = -1;
	if (getPromotionPiece(move) != PieceType::UNDEF) {
		const PieceType promotionType = getPromotionPiece(move);
		m_pieceOnTile[stTile]->type = promotionType;
		//Tell updateDynamicVars than pawn "disappeared" and then tell it that 
		//promotionType piece appeared from nothing on the same square
		if (m_blackToMove) {
			updateDynamicVars<1>(PieceType::PAWN, endTile, -1);
			updateDynamicVars<1>(promotionType, -1, endTile);
			blackEndgameWeight = -1;//Mark endgame weight as 'needs update'
		} else {
			updateDynamicVars<0>(PieceType::PAWN, endTile, -1);
			updateDynamicVars<0>(promotionType, -1, endTile);
			whiteEndgameWeight = -1;//Mark endgame weight as 'needs update'
		}
	}
	//En Passant
	int8_t captureTile = endTile, capturedPieceIdx = -1;
	if (m_pieceOnTile[stTile]->type == PieceType::PAWN && endTile == m_possibleEnPassant) {
		captureTile = endTile + (m_blackToMove ? 8 : -8);
	}
	//Capture
	if (m_pieceOnTile[captureTile] != nullptr) {
		const PieceType type = m_pieceOnTile[captureTile]->type;
		Piece** enemyPiece = (m_blackToMove ? m_whitePiece : m_blackPiece);
		const int8_t enemyPieceCnt = (m_blackToMove ? m_whiteTotalPiecesCnt : m_blackTotalPiecesCnt);
		//Update bitboards and zHash; Note: here flip <0> with <1>, 
		//because if black to move then white lost a piece
		if (m_blackToMove) {
			updateDynamicVars<0>(type, captureTile, -1);
			blackEndgameWeight = -1;//Mark endgame weight as 'needs update'
		} else {
			updateDynamicVars<1>(type, captureTile, -1);
			whiteEndgameWeight = -1;//Mark endgame weight as 'needs update'
		}
		capturedPieceIdx = m_pieceOnTile[captureTile]->idx;
		//Swap piece with last one in array (size decreased by 1 above)
		swap(enemyPiece[capturedPieceIdx], enemyPiece[enemyPieceCnt - 1]);
		//Update indices
		enemyPiece[capturedPieceIdx]->idx = capturedPieceIdx;
		enemyPiece[enemyPieceCnt - 1]->idx = enemyPieceCnt - 1;
		//Update pieceOnTile
		m_pieceOnTile[captureTile] = nullptr;
	}

	//Update En Passant target
	int8_t begginingEpTile = m_possibleEnPassant;
	if (m_pieceOnTile[stTile]->type == PieceType::PAWN && abs(stTile - endTile) == 16) {
		const uint64_t enemyPawnsBB = (m_blackToMove ? m_whiteBitboards : m_blackBitboards)[PAWN];
		uint64_t epPawnsMask = 0;
		if (getX(endTile) != 0) { epPawnsMask |= (1ULL << (endTile - 1)); }
		if (getX(endTile) != 7) { epPawnsMask |= (1ULL << (endTile + 1)); }
		if (epPawnsMask & enemyPawnsBB) {
			m_possibleEnPassant = (stTile + endTile) / 2;
		} else {
			m_possibleEnPassant = -1;
		}
	} else {
		m_possibleEnPassant = -1;
	}
	//Update zobrist hash for ep
	if (begginingEpTile != m_possibleEnPassant) {
		if (begginingEpTile != -1) {
			zHash ^= hashNumsEp[begginingEpTile & 0b111];
		}
		if (m_possibleEnPassant != -1) {
			zHash ^= hashNumsEp[m_possibleEnPassant & 0b111];
		}
	}


	//Update draw rules
	drawMan.rule50count++;
	drawMan.addGameState(zHash);
	if (m_pieceOnTile[stTile]->type == PAWN || m_pieceOnTile[captureTile] != nullptr) {
		drawMan.rule50count = 0;
	}

	//Set pieces on tiles
	m_pieceOnTile[stTile]->setPos(endTile);
	m_pieceOnTile[endTile] = m_pieceOnTile[stTile];
	m_pieceOnTile[stTile] = nullptr;
	m_blackToMove = !m_blackToMove;
	return capturedPieceIdx;
}

void Position::undoMove(int16_t move, int8_t capturedPieceIdx, int8_t bitmaskCastling_, int8_t possibleEnPassant_, int8_t rule50count_) {
	//Update draw rules
	drawMan.rule50count = rule50count_;
	drawMan.popGameState();

	//Update zobrist hash for castling and ep
	if (m_possibleEnPassant != -1) {
		zHash ^= hashNumsEp[getX(m_possibleEnPassant)];
	}
	if (possibleEnPassant_ != -1) {
		zHash ^= hashNumsEp[getX(possibleEnPassant_)];
	}
	zHash ^= hashNumsCastling[m_bitmaskCastling];
	zHash ^= hashNumsCastling[bitmaskCastling_];
	zHash ^= hashNumBlackToMove;

	//Revert castling, ep and blackToMove
	m_blackToMove = !m_blackToMove;
	m_bitmaskCastling = bitmaskCastling_;
	m_possibleEnPassant = possibleEnPassant_;
	int8_t stTile = getStartPos(move), endTile = getEndPos(move);
	//If promotion -> revert to a pawn
	if (getPromotionPiece(move) != PieceType::UNDEF) {
		const PieceType promotionType = m_pieceOnTile[endTile]->type;
		m_pieceOnTile[endTile]->type = PieceType::PAWN;
		//Tell updateDynamicVars than promotionType piece "disappeared" and 
		//then tell it that pawn appeared from nothing on the same square
		if (m_blackToMove) {
			updateDynamicVars<1>(promotionType, endTile, -1);
			updateDynamicVars<1>(PieceType::PAWN, -1, endTile);
			blackEndgameWeight = -1;//Mark endgame weight as 'needs update'
		} else {
			updateDynamicVars<0>(promotionType, endTile, -1);
			updateDynamicVars<0>(PieceType::PAWN, -1, endTile);
			whiteEndgameWeight = -1;//Mark endgame weight as 'needs update'
		}
	}
	//Check for castling (because also need to revert rook)
	if (m_pieceOnTile[endTile]->type == PieceType::KING && abs(endTile - stTile) == 2) {
		int8_t rookEndTile = (endTile + stTile) / 2;
		int8_t rookStTile = endTile + (endTile < stTile ? -2 : 1);
		m_pieceOnTile[rookEndTile]->setPos(rookStTile);
		m_pieceOnTile[rookStTile] = m_pieceOnTile[rookEndTile];
		m_pieceOnTile[rookEndTile] = nullptr;
		//Update rook bitboard, zHash and bonuses; Flip rookStTile and rookEndTile, because reverting
		if (m_blackToMove) { updateDynamicVars<1>(PieceType::ROOK, rookEndTile, rookStTile); }
		else { updateDynamicVars<0>(PieceType::ROOK, rookEndTile, rookStTile); }
	}
	//Update bitboards; Flip stTile and endTile, because revering move;
	//important to do this AFTER reverting promotions to have correct hash
	if (m_blackToMove) { updateDynamicVars<1>(m_pieceOnTile[endTile]->type, endTile, stTile); }
	else { updateDynamicVars<0>(m_pieceOnTile[endTile]->type, endTile, stTile); }
	//Update pieceOnTile;
	//its important to do it before undo-ing
	//captures to not overwrite piece with captured one
	m_pieceOnTile[endTile]->setPos(stTile);
	m_pieceOnTile[stTile] = m_pieceOnTile[endTile];
	m_pieceOnTile[endTile] = nullptr;

	//Undo captures
	int8_t captureTile = endTile;
	//Check for en passant
	if (m_pieceOnTile[stTile]->type == PieceType::PAWN && endTile == m_possibleEnPassant) {
		captureTile += (m_blackToMove ? 8 : -8);
	}
	if (capturedPieceIdx != -1) {
		Piece** enemyPiece = (m_blackToMove ? m_whitePiece : m_blackPiece);
		int8_t enemyPiecesCnt = (m_blackToMove ? m_whiteTotalPiecesCnt : m_blackTotalPiecesCnt);
		//Revert swap in makeMove
		swap(enemyPiece[capturedPieceIdx], enemyPiece[enemyPiecesCnt]);
		//Update indices
		enemyPiece[capturedPieceIdx]->idx = capturedPieceIdx;
		enemyPiece[enemyPiecesCnt]->idx = enemyPiecesCnt;
		//Update pieceOnTile
		m_pieceOnTile[captureTile] = enemyPiece[capturedPieceIdx];
		//Update bb, zHash and bonuses; swap <0> and <1>, because black to 
		//move == white lost a piece; stTile = -1, because this means revert capture
		PieceType type = m_pieceOnTile[captureTile]->type;
		if (m_blackToMove) {
			updateDynamicVars<0>(type, -1, captureTile);
			blackEndgameWeight = -1;//Mark endgame weight as 'needs update'
		} else {
			updateDynamicVars<1>(type, -1, captureTile);
			whiteEndgameWeight = -1;//Mark endgame weight as 'needs update'
		}
	}
}

void Position::makeNullMove() {
	m_blackToMove = !m_blackToMove;
	zHash ^= hashNumBlackToMove;
	if (m_possibleEnPassant != -1) {
		zHash ^= hashNumsEp[getX(m_possibleEnPassant)];
		m_possibleEnPassant = -1;
	}
}

void Position::undoNullMove(int8_t possibleEnPassant_) {
	m_blackToMove = !m_blackToMove;
	zHash ^= hashNumBlackToMove;
	//m_possibleEnPassant should be = -1
	if (possibleEnPassant_ != -1) {
		zHash ^= hashNumsEp[getX(possibleEnPassant_)];
	}
	m_possibleEnPassant = possibleEnPassant_;
}

void Position::deleteData() {
	if (m_whitePiece != nullptr) {
		for (int i = 0; i < m_whiteTotalPiecesCnt; i++) {
			delete m_whitePiece[i];
		}
		delete[] m_whitePiece;
	}
	if (m_blackPiece != nullptr) {
		for (int i = 0; i < m_blackTotalPiecesCnt; i++) {
			delete m_blackPiece[i];
		}
		delete[] m_blackPiece;
	}
	if (m_whitePiecesCnt != nullptr) {
		delete[] m_whitePiecesCnt;
	}
	if (m_blackPiecesCnt != nullptr) {
		delete[] m_blackPiecesCnt;
	}
	if (m_pieceOnTile != nullptr) {
		delete[] m_pieceOnTile;
	}
	if (m_whiteBitboards != nullptr) {
		delete[] m_whiteBitboards;
	}
	if (m_blackBitboards != nullptr) {
		delete[] m_blackBitboards;
	}
	if (m_legalMoves != nullptr) {
		delete[] m_legalMoves;
	}

	drawMan.reset();
}