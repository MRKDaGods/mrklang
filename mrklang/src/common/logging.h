#pragma once

#include "macros.h"

#include <iostream>
#include <sstream>
#include <source_location>
#include <format>

MRK_NS_BEGIN

enum class LogLevel {
	DEBUG,
	INFO,
	WARN,
	ERROR,
	FATAL
};

// Unbuffered logging
#define MRK_LOG(level, fmt, ...) \
    do { \
        if (static_cast<int>(level) >= static_cast<int>(MRK logThreshold)) { \
            auto msg = MRK_STD format( \
                "[{}] {}:{} - " fmt "\n", \
                #level, \
                MRK_STD source_location::current().file_name(), \
                MRK_STD source_location::current().line(), \
                ##__VA_ARGS__ \
            ); \
            MRK_STD cerr << msg; \
        } \
    } while (0)

#define MRK_DEBUG(fmt, ...) MRK_LOG(MRK LogLevel::DEBUG, fmt, ##__VA_ARGS__)
#define MRK_INFO(fmt, ...)  MRK_LOG(MRK LogLevel::INFO,  fmt, ##__VA_ARGS__)
#define MRK_WARN(fmt, ...)  MRK_LOG(MRK LogLevel::WARN,  fmt, ##__VA_ARGS__)
#define MRK_ERROR(fmt, ...) MRK_LOG(MRK LogLevel::ERROR, fmt, ##__VA_ARGS__)
#define MRK_FATAL(fmt, ...) MRK_LOG(MRK LogLevel::FATAL, fmt, ##__VA_ARGS__)

// Global log threshold
inline LogLevel logThreshold = LogLevel::INFO;

MRK_NS_END