#pragma once

#include "common/macros.h"

#include <stack>
#include <chrono>

MRK_NS_BEGIN

using ProfilerClock = std::chrono::high_resolution_clock;

class Profiler {
public:
	Profiler() = delete;
	Profiler(const Profiler&) = delete;
	Profiler& operator=(const Profiler&) = delete;

	static void start() {
		startTimes().push(ProfilerClock::now());
	}

	static std::chrono::milliseconds stop() {
		auto stopTime = ProfilerClock::now();
		auto startTime = startTimes().top();
		startTimes().pop();
		return std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime);
	}

private:
	static std::stack<ProfilerClock::time_point>& startTimes() {
		static std::stack<ProfilerClock::time_point> instance;
		return instance;
	}
};

MRK_NS_END