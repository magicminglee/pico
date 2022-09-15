#include "xlog.hpp"
#include "argument.hpp"

NAMESPACE_FRAMEWORK_BEGIN

static const int MAX_LOG_SIZE = 64 * 1024 * 1024;
static const int MAX_ROTATE_FILES = 10;

std::shared_ptr<spdlog::logger> XLOG::logger;

static const char* logLevelToString(XLOG::XGAMELogLevel l)
{
    switch (l) {
    case XLOG::XGAMELogLevel::XGAMELL_TRACE:
        return "TRACE";
    case XLOG::XGAMELogLevel::XGAMELL_DEBUG:
        return "DEBUG";
    case XLOG::XGAMELogLevel::XGAMELL_INFO:
        return "INFO";
    case XLOG::XGAMELogLevel::XGAMELL_WARNING:
        return "WARN";
    case XLOG::XGAMELogLevel::XGAMELL_ERROR:
        return "ERROR";
    case XLOG::XGAMELogLevel::XGAMELL_CRITICAL:
        return "CRITICAL";
    default:
        return "UNKNOWN";
    }
}

static XLOG::XGAMELogLevel logStrToLogLevel(std::string_view lvstr)
{
    if (lvstr == "TRACE")
        return XLOG::XGAMELogLevel::XGAMELL_TRACE;
    if (lvstr == "DEBUG")
        return XLOG::XGAMELogLevel::XGAMELL_DEBUG;
    if (lvstr == "INFO")
        return XLOG::XGAMELogLevel::XGAMELL_INFO;
    if (lvstr == "WARN")
        return XLOG::XGAMELogLevel::XGAMELL_WARNING;
    if (lvstr == "ERROR")
        return XLOG::XGAMELogLevel::XGAMELL_ERROR;
    if (lvstr == "CRITICAL")
        return XLOG::XGAMELogLevel::XGAMELL_CRITICAL;

    return XLOG::XGAMELogLevel::XGAMELL_INFO;
}

bool XLOG::LogInit()
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto rotate_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(MYARGS.LogFile.value_or("log.log"), MYARGS.RotateBytes.value_or(MAX_LOG_SIZE), MYARGS.RotateNums.value_or(MAX_ROTATE_FILES));
    spdlog::sinks_init_list sinklist = { console_sink, rotate_sink };
    logger = std::make_shared<spdlog::logger>(MYARGS.ModuleName.value_or(""), sinklist);
    logger->set_pattern("[%Y-%m-%d %T.%f] [%l] [%s:%#] => %v", spdlog::pattern_time_type::local);
    Reload();
    spdlog::set_default_logger(logger);
    SPDLOG_DEBUG("Log Init Success");
    return true;
}

bool XLOG::Reload()
{
    logger->set_level(static_cast<spdlog::level::level_enum>(logStrToLogLevel(MYARGS.LogLevel.value_or("INFO"))));
    logger->flush_on(static_cast<spdlog::level::level_enum>(logStrToLogLevel(MYARGS.FlushLevel.value_or("INFO"))));
    if (MYARGS.IsVerbose.value_or(false))
        logger->sinks().at(0)->set_level(static_cast<spdlog::level::level_enum>(logStrToLogLevel(MYARGS.LogLevel.value_or("INFO"))));
    else
        logger->sinks().at(0)->set_level(spdlog::level::off);
    return true;
}

NAMESPACE_FRAMEWORK_END