#include "Position.h"
#include "PrecomputedData.h"
#include "ZobristHashing.h"
using std::swap;
using std::abs;
int16_t* Position::m_legalMoves = nullptr;
const char* Position::m_startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

Position::Position() {
	m_blackToMove = 0;
	m_bitmaskCastling = 0;
	m_possibleEnPassant = -1;
	m_legalMovesStartIdx = 0;
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
	whiteEndgameWeight = 0;
	blackEndgameWeight = 0;
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
	m_possibleEnPassant = -1;
	m_whiteAllPiecesBitboard = 0;
	m_blackAllPiecesBitboard = 0;
	m_whiteTotalPiecesCnt = 0;
	m_blackTotalPiecesCnt = 0;
	m_bitmaskCastling = 0;
	m_blackToMove = 0;
	m_whitePiecesEval = 0;
	m_blackPiecesEval = 0;
	whiteEndgameWeight = 0;
	blackEndgameWeight = 0;
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
	for (i = i; fen[i]; i++) {
		if (fen[i] == ' ') {
			i++;
			break;
		}
		if (fen[i] == 'K') { m_bitmaskCastling |= 8; }
		if (fen[i] == 'Q') { m_bitmaskCastling |= 4; }
		if (fen[i] == 'k') { m_bitmaskCastling |= 2; }
		if (fen[i] == 'q') { m_bitmaskCastling |= 1; }
	}
	if (!(fen[i] == ' ' || fen[i] == '-')) {
		m_possibleEnPassant = 0;
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
		m_whiteSqBonusEval += getSqBonus(m_whitePiece[i]->type, m_whitePiece[i]->pos);
	}
	for (int8_t i = 0; i < m_blackTotalPiecesCnt; i++) {
		m_blackSqBonusEval += getSqBonus(m_blackPiece[i]->type, m_blackPiece[i]->pos);
	}
	//Set zobrist hash; Do it here for the same reason as setting indices
	zHash = getPositionHash(*this);
	//Set legal moves
	updateLegalMoves<0>();
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
		} else {
			updateDynamicVars<0>(PieceType::PAWN, endTile, -1);
			updateDynamicVars<0>(promotionType, -1, endTile);
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
		if (m_blackToMove) { updateDynamicVars<0>(type, captureTile, -1); }
		else { updateDynamicVars<1>(type, captureTile, -1); }
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
	if (m_pieceOnTile[stTile]->type == PieceType::PAWN && abs(getY(stTile) - getY(endTile)) != 1) {
		m_possibleEnPassant = (stTile + endTile) >> 1;
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

	//Set pieces on tiles
	m_pieceOnTile[stTile]->setPos(endTile);
	m_pieceOnTile[endTile] = m_pieceOnTile[stTile];
	m_pieceOnTile[stTile] = nullptr;
	m_blackToMove = !m_blackToMove;
	return capturedPieceIdx;
}

void Position::undoMove(int16_t move, int8_t capturedPieceIdx, int8_t bitmaskCastling_, int8_t possibleEnPassant_) {
	//Update zobrist hash for castling and ep
	if (m_possibleEnPassant != -1) {
		zHash ^= hashNumsEp[m_possibleEnPassant & 0b111];
	}
	if (possibleEnPassant_ != -1) {
		zHash ^= hashNumsEp[possibleEnPassant_ & 0b111];
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
		} else {
			updateDynamicVars<0>(promotionType, endTile, -1);
			updateDynamicVars<0>(PieceType::PAWN, -1, endTile);
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
		if (m_blackToMove) { updateDynamicVars<0>(type, -1, captureTile); }
		else { updateDynamicVars<1>(type, -1, captureTile); }
	}
}

void Position::initLegalMoves() {
	m_legalMoves = new int16_t[8192]();
	for (int i = 0; i < 8192; i++) {
		m_legalMoves[i] = createMove(0, 0, PieceType::UNDEF);
	}
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
}