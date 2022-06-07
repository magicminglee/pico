#pragma once

#include "event2/event.h"

#include "common.hpp"
#include "contex.hpp"
#include "object.hpp"

NAMESPACE_FRAMEWORK_BEGIN

class CSignal : public CObject {
public:
    CSignal() = default;
    ~CSignal() = default;

    DISABLE_CLASS_COPYABLE(CSignal);
};

NAMESPACE_FRAMEWORK_END