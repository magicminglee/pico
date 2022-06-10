#pragma once
#include <ctime>

#include "argument.hpp"
#include "utils.hpp"

NAMESPACE_FRAMEWORK_BEGIN

class CGlobal {
public:
    static void InitGlobal()
    {
        if (MYARGS.TimeZone)
            setenv("TZ", MYARGS.TimeZone.value().c_str(), 1);
        if (MYARGS.MaxFrameSize)
            MAX_WATERMARK_SIZE = MYARGS.MaxFrameSize.value();
    }
};

NAMESPACE_FRAMEWORK_END