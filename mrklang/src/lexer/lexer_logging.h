#pragma once
#include "common/logging.h"

MRK_NS_BEGIN

#define MRK_LOG_LEX(type, fmt, ...) MRK_##type(fmt " at line {} col {}", __VA_ARGS__, position_.line, position_.column)
#define MRK_DEBUG_LEX(fmt, ...) MRK_LOG_LEX(DEBUG, fmt, __VA_ARGS__)
#define MRK_INFO_LEX(fmt, ...)  MRK_LOG_LEX(INFO,  fmt, __VA_ARGS__)
#define MRK_WARN_LEX(fmt, ...)  MRK_LOG_LEX(WARN,  fmt, __VA_ARGS__)
#define MRK_ERROR_LEX(fmt, ...) MRK_LOG_LEX(ERROR, fmt, __VA_ARGS__)
#define MRK_FATAL_LEX(fmt, ...) MRK_LOG_LEX(FATAL, fmt, __VA_ARGS__)

MRK_NS_END