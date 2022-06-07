#pragma once
#include "framework/common.hpp"

//#include "gperftools/profiler.h"

USE_NAMESPACE_FRAMEWORK

namespace gamelibs {
namespace profiler {
    class CProfilerMgr : public CTLSingleton<CProfilerMgr> {
    public:
        CProfilerMgr() = default;

        ~CProfilerMgr()
        {
            //ProfilerStop();
        }

        void Start(const std::string_view filename)
        {
            //ProfilerStart(filename.data());
        }
    };
}
} //end namespace profiler