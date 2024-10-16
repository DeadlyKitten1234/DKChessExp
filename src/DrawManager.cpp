#include "DrawManager.h"
#include "Piece.h"

DrawManager::DrawManager() {
	repHistory = new uint64_t[1024]();
	repHistorySz = 0;
	rule50count = 0;
}

DrawManager::~DrawManager() {
	if (repHistory != nullptr) {
		delete[] repHistory;
	}
}

void DrawManager::addGameState(uint64_t zHash) {
	repHistory[repHistorySz++] = zHash;
}

bool DrawManager::checkForRep() const {
	if (repHistorySz <= 5) {
		return false;
	}
	const uint64_t hashToCompare = repHistory[repHistorySz - 1];
	bool foundOnce = false;
	for (int i = repHistorySz - 3; i >= repHistorySz - 1 - rule50count; i -= 2) {
		if (repHistory[i] == hashToCompare) {
			if (foundOnce) {
				return true;
			}
			foundOnce = true;
		}
	}
	return false;
}

bool DrawManager::checkLastStateRepTwice() const {
	if (repHistorySz <= 2) {
		return false;
	}
	const uint64_t hashToCompare = repHistory[repHistorySz - 1];
	for (int i = repHistorySz - 3; i >= repHistorySz - 1 - rule50count; i -= 2) {
		if (repHistory[i] == hashToCompare) {
			return true;
		}
	}
	return false;
}

bool DrawManager::insufMaterial(int8_t totalPcsCnt, int8_t* piecesCnt) const {
	if (totalPcsCnt == 1) {//Only king
		return true;
	}
	//King and bishop/knight
	if (totalPcsCnt == 2 && (piecesCnt[BISHOP] || piecesCnt[KNIGHT])) {
		return true;
	}
	return false;
}
