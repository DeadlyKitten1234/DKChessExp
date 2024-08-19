#include "DrawManager.h"

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

bool DrawManager::checkForRep() {
	if (repHistorySz <= 5) {
		return false;
	}
	const uint64_t hashToCompare = repHistory[repHistorySz - 1];
	bool foundOnce = false;
	for (int i = repHistorySz - 3; i >= repHistorySz - rule50count; i -= 2) {
		if (repHistory[i] == hashToCompare) {
			return true;
			if (foundOnce) {
				return true;
			}
			foundOnce = true;
		}
	}
	return false;
}
