#pragma once

#include <spdlog/spdlog.h>

#define HNOKER_LOGGER_NAME "hnoker_logger"
#define HNOKER_LOGGER_THREAD_COUNT 3

#define INFO(...) spdlog::get(HNOKER_LOGGER_NAME)->info(__VA_ARGS__);
#define WARN(...) spdlog::get(HNOKER_LOGGER_NAME)->warn(__VA_ARGS__);
#define CRITICAL(...) spdlog::get(HNOKER_LOGGER_NAME)->critical(__VA_ARGS__);
