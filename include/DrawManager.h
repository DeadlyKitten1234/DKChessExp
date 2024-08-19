#pragma once
#include <cinttypes>

class DrawManager {
public:
	DrawManager();
	~DrawManager();

	void addGameState(uint64_t zHash);
	void reset() { repHistorySz = 0; rule50count = 0; };
	void popGameState() { repHistorySz--; };
	bool checkForRule50() const { return rule50count >= 100; };
	bool checkForRep();

	int8_t rule50count;
private:
	int repHistorySz;
	uint64_t* repHistory;
};