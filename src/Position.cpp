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
			} else {
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
	}
	else {
		m_blackToMove = 0;
	}
	i += 2;
	for (i = i; fen[i]; i++) {
		if (fen[i] == ' ') {
			i++;
			break;
		}
		if (fen[i] == 'K') {
			m_bitmaskCastling |= 8;
		}
		if (fen[i] == 'Q') {
			m_bitmaskCastling |= 4;
		}
		if (fen[i] == 'k') {
			m_bitmaskCastling |= 2;
		}
		if (fen[i] == 'q') {
			m_bitmaskCastling |= 1;
		}
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

	zHash = getPositionHash(*this);
	updateLegalMoves<0>();
}

int8_t Position::makeMove(int16_t move) {
	int8_t stTile = getStartPos(move), endTile = getEndPos(move);
	//Update bitboards
	if (m_blackToMove) {
		PieceType type = m_pieceOnTile[stTile]->type;
		m_blackAllPiecesBitboard &= ~(1ULL << stTile);
		m_blackAllPiecesBitboard |= (1ULL << endTile);
		m_blackBitboards[type] &= ~(1ULL << stTile);
		m_blackBitboards[type] |= (1ULL << endTile);
	} else {
		PieceType type = m_pieceOnTile[stTile]->type;
		m_whiteAllPiecesBitboard &= ~(1ULL << stTile);
		m_whiteAllPiecesBitboard |= (1ULL << endTile);
		m_whiteBitboards[type] &= ~(1ULL << stTile);
		m_whiteBitboards[type] |= (1ULL << endTile);
	}
	//Update zobrist hash
	zHash ^= hashNums[stTile][m_blackToMove][m_pieceOnTile[stTile]->type];
	zHash ^= hashNums[endTile][m_blackToMove][m_pieceOnTile[stTile]->type];
	zHash ^= hashNumBlackToMove;
	//If castle
	if (m_pieceOnTile[stTile]->type == PieceType::KING && abs(stTile - endTile) == 2) {
		int8_t addRookPos = (stTile > endTile ? 1 : -1);
		int8_t rookPos = stTile + (addRookPos == 1 ? -4 : 3);
		m_pieceOnTile[rookPos]->setPos(endTile + addRookPos);
		m_pieceOnTile[endTile + addRookPos] = m_pieceOnTile[rookPos];
		m_pieceOnTile[rookPos] = nullptr;
		//Update rook bitboard
		if (m_blackToMove) {
			m_blackAllPiecesBitboard &= ~(1ULL << rookPos);
			m_blackAllPiecesBitboard |= (1ULL << (endTile + addRookPos));
			m_blackBitboards[ROOK] &= ~(1ULL << rookPos);
			m_blackBitboards[ROOK] |= (1ULL << (endTile + addRookPos));
		} else {
			m_whiteAllPiecesBitboard &= ~(1ULL << rookPos);
			m_whiteAllPiecesBitboard |= (1ULL << (endTile + addRookPos));
			m_whiteBitboards[ROOK] &= ~(1ULL << rookPos);
			m_whiteBitboards[ROOK] |= (1ULL << (endTile + addRookPos));
		}
		//Update rook zobrist hash
		zHash ^= hashNums[rookPos][m_blackToMove][ROOK];
		zHash ^= hashNums[endTile + addRookPos][m_blackToMove][ROOK];
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
	if (endTile == 0) { m_bitmaskCastling &= ~4; }//~0100
	if (endTile == 7) { m_bitmaskCastling &= ~8; }//~1000
	if (endTile == 56) { m_bitmaskCastling &= ~1; }//~0001
	if (endTile == 63) { m_bitmaskCastling &= ~2; }//~0010
	//Update zobrist hash for castling
	zHash ^= hashNumsCastling[begginingCastleRights];
	zHash ^= hashNumsCastling[m_bitmaskCastling];
	//Promote
	int8_t promotedPawnIdx = -1;
	if (getPromotionPiece(move) != PieceType::UNDEF) {
		PieceType promotionType = getPromotionPiece(move);
		m_pieceOnTile[stTile]->type = promotionType;
		//Update bitboards
		if (m_blackToMove) {
			m_blackBitboards[PAWN] &= ~(1ULL << endTile);
			m_blackBitboards[promotionType] |= (1ULL << endTile);
			m_blackPiecesCnt[promotionType]++;
			m_blackPiecesCnt[PAWN]--;
			m_blackPiecesEval += pieceValue[promotionType] - pieceValue[PAWN];
		} else {
			m_whiteBitboards[PAWN] &= ~(1ULL << endTile);
			m_whiteBitboards[promotionType] |= (1ULL << endTile);
			m_whitePiecesCnt[promotionType]++;
			m_whitePiecesCnt[PAWN]--;
			m_whitePiecesEval += pieceValue[promotionType] - pieceValue[PAWN];
		}
		//Update zobrist hash
		zHash ^= hashNums[endTile][m_blackToMove][PAWN];
		zHash ^= hashNums[endTile][m_blackToMove][promotionType];
	}
	//En Passant
	int8_t captureTile = endTile, capturedPieceIdx = -1;
	if (m_pieceOnTile[stTile]->type == PieceType::PAWN && endTile == m_possibleEnPassant) {
		captureTile = endTile + (m_blackToMove ? 8 : -8);
	}
	//Capture
	if (m_pieceOnTile[captureTile] != nullptr) {
		PieceType type = m_pieceOnTile[captureTile]->type;
		Piece** enemyPiece = (m_blackToMove ? m_whitePiece : m_blackPiece);
		int8_t enemyPieceCnt = (m_blackToMove ? m_whiteTotalPiecesCnt : m_blackTotalPiecesCnt);

		for (int8_t i = 0; i < enemyPieceCnt; i++) {
			if (enemyPiece[i] == m_pieceOnTile[captureTile]) {
				if (m_blackToMove) {
					m_whiteTotalPiecesCnt--;
					m_whitePiecesCnt[type]--;
					m_whiteBitboards[type] &= ~(1ULL << captureTile);
					m_whiteAllPiecesBitboard &= ~(1ULL << captureTile);
					m_whitePiecesEval -= pieceValue[type];
				} else {
					m_blackTotalPiecesCnt--;
					m_blackPiecesCnt[type]--;
					m_blackBitboards[type] &= ~(1ULL << captureTile);
					m_blackAllPiecesBitboard &= ~(1ULL << captureTile);
					m_blackPiecesEval -= pieceValue[type];
				}
				swap(enemyPiece[i], enemyPiece[enemyPieceCnt - 1]);
				capturedPieceIdx = i;
				break;
			}
		}
		m_pieceOnTile[captureTile] = nullptr;
		//Update zobrist hash
		zHash ^= hashNums[captureTile][!m_blackToMove][type];
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

void Position::undoMove(int16_t move, int8_t capturedPieceIdx, int8_t bitmaskCastling, int8_t possibleEnPassant) {
	//Update zobrist hash for castling and ep
	if (m_possibleEnPassant != -1) {
		zHash ^= hashNumsEp[m_possibleEnPassant & 0b111];
	}
	if (possibleEnPassant != -1) {
		zHash ^= hashNumsEp[possibleEnPassant & 0b111];
	}
	zHash ^= hashNumsCastling[m_bitmaskCastling];
	zHash ^= hashNumsCastling[bitmaskCastling];
	zHash ^= hashNumBlackToMove;

	//Revert castling, ep and blackToMove
	m_blackToMove = !m_blackToMove;
	m_bitmaskCastling = bitmaskCastling;
	m_possibleEnPassant = possibleEnPassant;
	int8_t stTile = getStartPos(move), endTile = getEndPos(move);
	//Update bitboards
	if (m_blackToMove) {
		PieceType type = m_pieceOnTile[endTile]->type;
		m_blackAllPiecesBitboard |= (1ULL << stTile);
		m_blackAllPiecesBitboard &= ~(1ULL << endTile);
		m_blackBitboards[type] |= (1ULL << stTile);
		m_blackBitboards[type] &= ~(1ULL << endTile);
	} else {
		PieceType type = m_pieceOnTile[endTile]->type;
		m_whiteAllPiecesBitboard |= (1ULL << stTile);
		m_whiteAllPiecesBitboard &= ~(1ULL << endTile);
		m_whiteBitboards[type] |= (1ULL << stTile);
		m_whiteBitboards[type] &= ~(1ULL << endTile);
	}
	//If promotion -> revert to a pawn
	if (getPromotionPiece(move) != PieceType::UNDEF) {
		PieceType promotionType = m_pieceOnTile[endTile]->type;
		m_pieceOnTile[endTile]->type = PieceType::PAWN;
		//Update bitboards and pieces cnt
		if (m_blackToMove) {
			m_blackBitboards[PAWN] |= (1ULL << stTile);
			m_blackBitboards[promotionType] &= ~(1ULL << stTile);
			m_blackPiecesCnt[promotionType]--;
			m_blackPiecesCnt[PAWN]++;
			m_blackPiecesEval += pieceValue[PAWN] - pieceValue[promotionType];
		} else {
			m_whiteBitboards[PAWN] |= (1ULL << stTile);
			m_whiteBitboards[promotionType] &= ~(1ULL << stTile);
			m_whitePiecesCnt[promotionType]--;
			m_whitePiecesCnt[PAWN]++;
			m_whitePiecesEval += pieceValue[PAWN] - pieceValue[promotionType];
		}
		//Update zobrist hash
		zHash ^= hashNums[endTile][m_blackToMove][promotionType];
		zHash ^= hashNums[endTile][m_blackToMove][PAWN];
	}
	//Check for castling (because also need to revert rook)
	if (m_pieceOnTile[endTile]->type == PieceType::KING && abs(endTile - stTile) == 2) {
		int8_t rookEndTile = (endTile + stTile) / 2;
		int8_t rookStTile = endTile + (endTile < stTile ? -2 : 1);
		m_pieceOnTile[rookEndTile]->setPos(rookStTile);
		m_pieceOnTile[rookStTile] = m_pieceOnTile[rookEndTile];
		m_pieceOnTile[rookEndTile] = nullptr;
		//Update rook bitboard
		if (m_blackToMove) {
			m_blackAllPiecesBitboard &= ~(1ULL << rookEndTile);
			m_blackAllPiecesBitboard |= (1ULL << rookStTile);
			m_blackBitboards[ROOK] &= ~(1ULL << rookEndTile);
			m_blackBitboards[ROOK] |= (1ULL << rookStTile);
		} else {
			m_whiteAllPiecesBitboard &= ~(1ULL << rookEndTile);
			m_whiteAllPiecesBitboard |= (1ULL << rookStTile);
			m_whiteBitboards[ROOK] &= ~(1ULL << rookEndTile);
			m_whiteBitboards[ROOK] |= (1ULL << rookStTile);
		}
		//Update rook zobrist hash
		zHash ^= hashNums[rookStTile][m_blackToMove][ROOK];
		zHash ^= hashNums[rookEndTile][m_blackToMove][ROOK];
	}
	//Update zobrist hash; important to do this AFTER reverting promotions to have correct hash
	zHash ^= hashNums[stTile][m_blackToMove][m_pieceOnTile[endTile]->type];
	zHash ^= hashNums[endTile][m_blackToMove][m_pieceOnTile[endTile]->type];
	//Update pieceOnTile;
	//its important to do it before undo-ing
	//captures to not overwrite piece with captured one
	m_pieceOnTile[endTile]->setPos(stTile);
	m_pieceOnTile[stTile] = m_pieceOnTile[endTile];
	m_pieceOnTile[endTile] = nullptr;

	//Undo captures
	int8_t captureTile = endTile;
	//Check for en passant
	if (m_pieceOnTile[stTile]->type == PieceType::PAWN && endTile == possibleEnPassant) {
		captureTile += (m_blackToMove ? 8 : -8);
	}
	if (capturedPieceIdx != -1) {
		Piece** enemyPiece = (m_blackToMove ? m_whitePiece : m_blackPiece);
		int8_t enemyPiecesCnt = (m_blackToMove ? m_whiteTotalPiecesCnt : m_blackTotalPiecesCnt);
		swap(enemyPiece[capturedPieceIdx], enemyPiece[enemyPiecesCnt]);
		m_pieceOnTile[captureTile] = enemyPiece[capturedPieceIdx];
		PieceType type = m_pieceOnTile[captureTile]->type;
		if (m_blackToMove) {
			m_whiteTotalPiecesCnt++;
			m_whitePiecesCnt[type]++;
			m_whiteBitboards[type] |= (1ULL << captureTile);
			m_whiteAllPiecesBitboard |= (1ULL << captureTile);
			m_whitePiecesEval += pieceValue[type];
		} else {
			m_blackTotalPiecesCnt++;
			m_blackPiecesCnt[type]++;
			m_blackBitboards[type] |= (1ULL << captureTile);
			m_blackAllPiecesBitboard |= (1ULL << captureTile);
			m_blackPiecesEval += pieceValue[type];
		}
		//Update zobrist hash
		zHash ^= hashNums[captureTile][!m_blackToMove][type];
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