#pragma once
#include <chrono>
#include <iostream>
#include <cinttypes>
using std::cout;

namespace Clock {
	inline int64_t now() {
		return	std::chrono::duration_cast<std::chrono::milliseconds>
				(std::chrono::steady_clock::now().time_since_epoch()).count();
	}
	inline void printTime(int64_t a) {
		bool hours = 0;
		cout << a / 3600000 << ':';
		a %= 3600000;
		cout << a / 600000 << (a % 600000) / 60000 << ':';
		a %= 60000;
		cout << a / 10000 << (a % 10000) / 1000;
		a %= 1000;
		if (a != 0) {
			cout << '.' << a / 100 << (a % 100) / 10 << a / 100;
		}
		cout << '\n';
	}
}