#pragma once
#include <chrono>
#include <cinttypes>

namespace Clock {
	inline int64_t now() {
		return	std::chrono::duration_cast<std::chrono::milliseconds>
				(std::chrono::steady_clock::now().time_since_epoch()).count();
	}
}