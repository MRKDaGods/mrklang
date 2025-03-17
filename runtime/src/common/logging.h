#pragma once

#ifndef MRK_COMMON_LOGGING_H
#define MRK_COMMON_LOGGING_H

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
#ifdef PRINT_FILE_NAMES
#define MRK_LOG(level, fmt, ...) \
    do { \
        if (static_cast<int>(level) >= static_cast<int>(mrklang::logThreshold)) { \
            auto msg = std::format( \
                "[{}] {}:{} - " fmt "\n", \
                #level, \
                std::source_location::current().file_name(), \
                std::source_location::current().line(), \
                ##__VA_ARGS__ \
            ); \
            std::cerr << msg; \
        } \
    } while (0)
#else
#define MRK_LOG(level, fmt, ...) \
    do { \
        if (static_cast<int>(level) >= static_cast<int>(mrklang::logThreshold)) { \
            auto msg = std::format( \
                "[{}] - " fmt "\n", \
                #level, \
                ##__VA_ARGS__ \
            ); \
            std::cerr << msg; \
        } \
    } while (0)
#endif

#define MRK_DEBUG(fmt, ...) MRK_LOG(mrklang::LogLevel::DEBUG, fmt __VA_OPT__(,) ##__VA_ARGS__)
#define MRK_INFO(fmt, ...)  MRK_LOG(mrklang::LogLevel::INFO,  fmt __VA_OPT__(,) ##__VA_ARGS__)
#define MRK_WARN(fmt, ...)  MRK_LOG(mrklang::LogLevel::WARN,  fmt __VA_OPT__(,) ##__VA_ARGS__)
#define MRK_ERROR(fmt, ...) MRK_LOG(mrklang::LogLevel::ERROR, fmt __VA_OPT__(,) ##__VA_ARGS__)
#define MRK_FATAL(fmt, ...) MRK_LOG(mrklang::LogLevel::FATAL, fmt __VA_OPT__(,) ##__VA_ARGS__)

// Global log threshold
inline LogLevel logThreshold = LogLevel::INFO;

MRK_NS_END

#endif