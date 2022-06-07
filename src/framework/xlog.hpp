#pragma once

#include <stdio.h>

#define CERROR(log_fmt, ...)                                                             \
    do {                                                                                 \
        XLOG::WARN_W.Log(XLOG::XGAMELogLevel::XGAMELL_ERROR, "[%s:%d] => " log_fmt "\n", \
            __FILE__, __LINE__, ##__VA_ARGS__);                                          \
        XLOG::INFO_W.Log(XLOG::XGAMELogLevel::XGAMELL_INFO, "[%s:%d] => " log_fmt "\n",  \
            __FILE__, __LINE__, ##__VA_ARGS__);                                          \
    } while (0)

#define CWARN(log_fmt, ...)                                                                \
    do {                                                                                   \
        XLOG::WARN_W.Log(XLOG::XGAMELogLevel::XGAMELL_WARNING, "[%s:%d] => " log_fmt "\n", \
            __FILE__, __LINE__, ##__VA_ARGS__);                                            \
    } while (0)

#define CTRACE(log_fmt, ...)                                                             \
    do {                                                                                 \
        XLOG::INFO_W.Log(XLOG::XGAMELogLevel::XGAMELL_TRACE, "[%s:%d] => " log_fmt "\n", \
            __FILE__, __LINE__, ##__VA_ARGS__);                                          \
    } while (0)

#define CINFO(log_fmt, ...)                                                             \
    do {                                                                                \
        XLOG::INFO_W.Log(XLOG::XGAMELogLevel::XGAMELL_INFO, "[%s:%d] => " log_fmt "\n", \
            __FILE__, __LINE__, ##__VA_ARGS__);                                         \
    } while (0)

#define CDEBUG(log_fmt, ...)                                                             \
    do {                                                                                 \
        XLOG::INFO_W.Log(XLOG::XGAMELogLevel::XGAMELL_DEBUG, "[%s:%d] => " log_fmt "\n", \
            __FILE__, __LINE__, ##__VA_ARGS__);                                          \
    } while (0)

#define CBIGLOG(data, log_fmt, ...)                                                         \
    do {                                                                                    \
        XLOG::INFO_W.BigLog(XLOG::XGAMELogLevel::XGAMELL_INFO, data, "[%s:%d] => " log_fmt, \
            __FILE__, __LINE__, ##__VA_ARGS__);                                             \
    } while (0)


namespace XLOG {
enum class XGAMELogLevel {
    XGAMELL_TRACE = 1,
    XGAMELL_DEBUG = 2,
    XGAMELL_INFO = 3,
    XGAMELL_WARNING = 4,
    XGAMELL_ERROR = 5,
};

class LogWriter {
public:
    LogWriter() = default;
    ~LogWriter();
    //loginit
    //@param l: log level
    //@param dirname: the directory of log
    //@param modulename: log module name
    //@param verbose: is to print logs to terminal
    //@param isolate: is to isolate log by file size
    //@param append: is to append mode to write logs to file
    //@param issync: is to flush to disk per write system call
    bool LogInit(XGAMELogLevel l, const char* dirname, const char* modulename, bool verbose, bool isolate, bool append, bool issync);
    bool Log(XGAMELogLevel l, const char* logformat, ...) __attribute__((format(printf, 3, 4)));
    bool BigLog(XGAMELogLevel l, const std::string_view data, const char* logformat, ...) __attribute__((format(printf, 4, 5)));
    constexpr bool HasInit() { return fp != nullptr; }
    constexpr void RemoveConsole() { m_verbose = false; }

public:
    const static unsigned int M_LOG_PATH_LEN = 250;
    const static unsigned int M_LOG_MODULE_LEN = 32;

private:
    constexpr bool checklevel(XGAMELogLevel l) { return l >= m_system_level ? true : false; }
    int premakestr(char* buffer, XGAMELogLevel l);
    bool write(char* pbuffer, int len);
    bool logclose();
    constexpr void setLastlogday(int day) { m_lastlogday = day; }

private:
    const static unsigned int M_LOG_BUFFSIZE = 4 * 1024 * 1024;
    enum XGAMELogLevel m_system_level = { XGAMELogLevel::XGAMELL_INFO };
    FILE* fp = { nullptr };
    bool m_issync = { false };
    bool m_isappend = { true };
    bool m_verbose = { false };
    char m_dirpath[M_LOG_PATH_LEN];
    char m_modulename[M_LOG_MODULE_LEN];
    int m_lastlogday = { -1 };
    bool m_isolate = { true };
    char* m_buffer = { nullptr };
};

//全局默认的日志写入器
inline thread_local LogWriter WARN_W;
inline thread_local LogWriter INFO_W;
inline thread_local LogWriter DBLOG_W;

bool LogInit(std::optional<std::string_view> l,
    std::optional<std::string_view> modulename,
    std::optional<std::string_view> logdir,
    std::optional<bool> verbose,
    std::optional<bool> isolate);
}