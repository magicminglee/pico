#pragma once
#include <chrono>
#include <iomanip>
#include <ratio>
#include <sstream>

#include "fmt/chrono.h"

NAMESPACE_FRAMEWORK_BEGIN

using namespace std::chrono;

static const std::string S_INVALID_TIME = "INVALID_TIME";

class CChrono {
    typedef time_point<system_clock, seconds> seconds_type;
    typedef time_point<system_clock, milliseconds> milliseconds_type;
    typedef time_point<system_clock, microseconds> microsecond_type;

public:
    static int64_t NowSec()
    {
        system_clock::time_point now = system_clock::now();
        return time_point_cast<seconds>(now).time_since_epoch().count();
    }

    static int64_t NowMs()
    {
        system_clock::time_point now = system_clock::now();
        return time_point_cast<milliseconds>(now).time_since_epoch().count();
    }

    static int64_t NowUs()
    {
        system_clock::time_point now = system_clock::now();
        return time_point_cast<microseconds>(now).time_since_epoch().count();
    }

    static const int64_t ZeroClock()
    {
        std::tm tm = std::tm();
        std::time_t tt = system_clock::to_time_t(system_clock::now());
        if (!LocalTime(&tt, &tm))
            return 0;

        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        tm.tm_isdst = -1;
        return std::mktime(&tm);
    }

    static struct tm* GMTime(const time_t* clock, struct tm* result)
    {
        if (nullptr == result)
            return nullptr;
        int64_t ns;
        if (!clock) {
            ns = NowSec();
        } else {
            ns = *clock;
        }
        *result = fmt::gmtime(ns);
        return result;
    }

    static struct tm* LocalTime(const time_t* clock, struct tm* result)
    {
        if (nullptr == result)
            return nullptr;
        int64_t ns;
        if (!clock) {
            ns = NowSec();
        } else {
            ns = *clock;
        }
        *result = fmt::localtime(ns);
        return result;
    }

    static char* AscTime(const struct tm* clock, char* result, size_t len)
    {
        if (nullptr == result || len <= 0)
            return nullptr;
#ifdef WINDOWS_PLATFORMOS
        return 0 == asctime_s(result, len, clock) ? result : nullptr;
#else
        return asctime_r(clock, result) ? result : nullptr;
#endif
    }

    static char* CTime(const time_t* clock, char* result, size_t len)
    {
        if (nullptr == result || len <= 0)
            return nullptr;
#ifdef WINDOWS_PLATFORMOS
        return 0 == ctime_s(result, len, clock) ? result : nullptr;
#else
        return ctime_r(clock, result) ? result : nullptr;
#endif
    }

    static time_t GetTime(const std::string& str, const char* fmt)
    {
        if (nullptr == fmt)
            return 0;
        std::tm tm = std::tm();
        int yy, month, dd, hh, mm, ss;
#ifdef WINDOWS_PLATFORMOS
        sscanf_s(str.c_str(), fmt, &yy, &month, &dd, &hh, &mm, &ss);
#else
        sscanf(str.c_str(), fmt, &yy, &month, &dd, &hh, &mm, &ss);
#endif
        tm.tm_year = yy - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = dd;
        tm.tm_hour = hh;
        tm.tm_min = mm;
        tm.tm_sec = ss;
        tm.tm_isdst = -1;
        return std::mktime(&tm);
    }

    static size_t PutTime(const char* fmt, char* buf, size_t len)
    {
        if (nullptr == buf || len <= 0)
            return 0;
        std::tm tm = std::tm();
        std::time_t tt = system_clock::to_time_t(system_clock::now());
        if (!LocalTime(&tt, &tm))
            return 0;
        return strftime(buf, len, fmt, &tm);
    }
};

NAMESPACE_FRAMEWORK_END
