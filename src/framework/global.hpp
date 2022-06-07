#pragma once
#include <ctime>

#include "argument.hpp"

NAMESPACE_FRAMEWORK_BEGIN

class CGlobal {
public:
    static void InitGlobal()
    {
        if (MYARGS.TimeZone)
            setenv("TZ", MYARGS.TimeZone.value().c_str(), 1);
    }
};

NAMESPACE_FRAMEWORK_END