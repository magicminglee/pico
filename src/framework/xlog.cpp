#include <algorithm>

#include <chrono>
#include <errno.h>
#include <filesystem>
#include <iomanip>
#include <optional>
#include <ratio>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>

#include "xlog.hpp"

namespace fs = std::filesystem;
namespace XLOG {

static const int MAX_LOG_SIZE = 64 * 1024 * 1024;
static const char* logLevelToString(XGAMELogLevel l)
{
    switch (l) {
    case XGAMELogLevel::XGAMELL_DEBUG:
        return "DEBUG";
    case XGAMELogLevel::XGAMELL_INFO:
        return "INFO";
    case XGAMELogLevel::XGAMELL_TRACE:
        return "TRACE";
    case XGAMELogLevel::XGAMELL_WARNING:
        return "WARN";
    case XGAMELogLevel::XGAMELL_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

XGAMELogLevel LogWriter::LogStrToLogLevel(std::string_view lvstr)
{
    if (lvstr == "DEBUG")
        return XGAMELogLevel::XGAMELL_DEBUG;
    if (lvstr == "INFO")
        return XGAMELogLevel::XGAMELL_INFO;
    if (lvstr == "TRACE")
        return XGAMELogLevel::XGAMELL_TRACE;
    if (lvstr == "WARN")
        return XGAMELogLevel::XGAMELL_WARNING;
    if (lvstr == "ERROR")
        return XGAMELogLevel::XGAMELL_ERROR;

    return XGAMELogLevel::XGAMELL_INFO;
}

bool LogInit(std::optional<std::string_view> l,
    std::optional<std::string_view> modulename,
    std::optional<std::string_view> logdir,
    std::optional<bool> verbose,
    std::optional<bool> isolate)
{
    INFO_W.LogInit(LogWriter::LogStrToLogLevel(l.value_or("INFO")), (std::string(logdir.value_or("./mylog")) + "/info/").c_str(), modulename.value().data(), verbose.value_or(false), isolate.value_or(true), true, false);
    WARN_W.LogInit(LogWriter::LogStrToLogLevel(l.value_or("WARN")), (std::string(logdir.value_or("./mylog")) + "/error/").c_str(), modulename.value().data(), verbose.value_or(false), isolate.value_or(true), true, false);

    return true;
}

LogWriter::~LogWriter()
{
    logclose();
    if (m_buffer)
        free(m_buffer);
}

bool LogWriter::LogInit(XGAMELogLevel l, const char* dirpath, const char* modulename, bool verbose, bool isolate, bool append, bool issync)
{
    if (!fs::exists(fs::status(dirpath))) {
        char cmd[256] = { 0 };
        snprintf(cmd, sizeof(cmd), "mkdir -p %s", dirpath);
        std::system(cmd);
    }
    if (!m_buffer)
        m_buffer = (char*)malloc(M_LOG_BUFFSIZE);
    time_t now = time(&now);
    struct tm vtm;
    localtime_r(&now, &vtm);
    size_t len = std::min(strlen(modulename), sizeof(m_modulename) - 1);
    strncpy(m_modulename, modulename, len);
    m_modulename[len] = '\0';
    len = std::min(strlen(dirpath), sizeof(m_dirpath) - 1);
    strncpy(m_dirpath, dirpath, sizeof(m_dirpath) - 1);
    m_dirpath[len] = '\0';

    m_system_level = l;
    m_isappend = append;
    m_issync = issync;
    m_verbose = verbose;
    m_isolate = isolate;

    char filelocation[M_LOG_PATH_LEN];
    memset(filelocation, 0, sizeof(filelocation));
    snprintf(filelocation, sizeof(filelocation), "%s/%s.%d%02d%02d-%02d-%02d", dirpath, modulename, 1900 + vtm.tm_year, vtm.tm_mon + 1, vtm.tm_mday, vtm.tm_hour, vtm.tm_min + 1);
    logclose();
    fp = fopen(filelocation, append ? "a" : "w");
    if (fp == nullptr) {
        fprintf(stderr, "cannot open log file,file location is %s\n", filelocation);
        return false;
    }
    setvbuf(fp, LogWriter::m_buffer, _IOLBF, M_LOG_BUFFSIZE); // buf set _IONBF  _IOLBF  _IOFBF
    // setvbuf (fp,  (char *)nullptr, _IOLBF, 0);
    // fprintf(stderr, "now all the running-information are going to the file %s\n", filelocation);
    setLastlogday(vtm.tm_mday);
    return true;
}

int LogWriter::premakestr(char* buffer, XGAMELogLevel l)
{
    time_t now;
    now = time(&now);
    struct tm vtm;
    localtime_r(&now, &vtm);

    struct timespec tv;
    timespec_get(&tv, TIME_UTC);

    return snprintf(buffer, M_LOG_BUFFSIZE, "[%d-%02d-%02d %02d:%02d:%02d.%06lu] %s ",
        1900 + vtm.tm_year, vtm.tm_mon + 1, vtm.tm_mday, vtm.tm_hour, vtm.tm_min, vtm.tm_sec,
        tv.tv_nsec / 1000, logLevelToString(l));
}

bool LogWriter::Log(XGAMELogLevel l, const char* logformat, ...)
{
    if (!checklevel(l))
        return false;

    if (!m_buffer)
        m_buffer = (char*)malloc(M_LOG_BUFFSIZE);

    char* start = m_buffer;
    int size = premakestr(start, l);

    va_list args;
    va_start(args, logformat);
    size += vsnprintf(start + size, M_LOG_BUFFSIZE - size, logformat, args);
    va_end(args);

    if (m_verbose)
        fprintf(stderr, "%s", m_buffer);

    if (fp) {
        time_t now = time(&now);
        struct tm vtm;
        localtime_r(&now, &vtm);
        // rotation by log file size or day time
        if (m_isolate && ((ftell(fp) >= MAX_LOG_SIZE) /* || (vtm.tm_mday != m_lastlogday)*/)) {
            LogInit(l, m_dirpath, m_modulename, m_verbose, m_isolate, m_isappend, m_issync);
        }
        write(m_buffer, size);
    }
    return true;
}

bool LogWriter::BigLog(XGAMELogLevel l, const std::string_view data, const char* logformat, ...)
{
    if (!checklevel(l))
        return false;

    if (!m_buffer)
        m_buffer = (char*)malloc(M_LOG_BUFFSIZE);
    char* start = m_buffer;
    int size = premakestr(start, l);

    va_list args;
    va_start(args, logformat);
    size += vsnprintf(start + size, M_LOG_BUFFSIZE - size, logformat, args);
    va_end(args);

    memcpy(start + size, data.data(), data.size());
    size += data.size();

    size += snprintf(start + size, M_LOG_BUFFSIZE - size, "%s", "\r\n");

    if (m_verbose)
        fprintf(stderr, "%s", m_buffer);

    if (fp) {
        time_t now = time(&now);
        struct tm vtm;
        localtime_r(&now, &vtm);
        // rotation by log file size or day time
        if (m_isolate && ((ftell(fp) >= MAX_LOG_SIZE) /* || (vtm.tm_mday != m_lastlogday)*/)) {
            LogInit(l, m_dirpath, m_modulename, m_verbose, m_isolate, m_isappend, m_issync);
        }
        write(m_buffer, size);
    }
    return true;
}

bool LogWriter::write(char* pbuffer, int len)
{
    if (1 == fwrite(pbuffer, len, 1, fp)) {
        if (m_issync)
            fflush(fp);
    } else {
        int x = errno;
        fprintf(stderr, "Failed to write to logfile. errno:%s message:%s", strerror(x), pbuffer);
        return false;
    }
    return true;
}

bool LogWriter::logclose()
{
    if (fp == nullptr)
        return false;
    fflush(fp);
    fclose(fp);
    fp = nullptr;
    return true;
}
}
