#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include "common.hpp"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

NAMESPACE_FRAMEWORK_BEGIN

class XLOG {
public:
    enum class XGAMELogLevel {
        XGAMELL_TRACE = 0,
        XGAMELL_DEBUG = 1,
        XGAMELL_INFO = 2,
        XGAMELL_WARNING = 3,
        XGAMELL_ERROR = 4,
        XGAMELL_CRITICAL = 5
    };

    static bool LogInit();
    static bool Reload();
    static std::shared_ptr<spdlog::logger> logger;
};
NAMESPACE_FRAMEWORK_END