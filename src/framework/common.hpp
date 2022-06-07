#pragma once
#include <assert.h>
#include <atomic>
#include <charconv>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <signal.h>
#include <string.h>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

#ifdef LINUX_PLATFORMOS
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <strings.h>
#include <unistd.h>
#endif

#ifndef PLATFORMOS
#error PLATFORMOS "NO PLATFORMOS SUPPORT"
#endif

#ifndef NAMESPACE_FRAMEWORK_BEGIN
#define NAMESPACE_FRAMEWORK_BEGIN namespace FRAMEWORK {
#endif

#ifndef NAMESPACE_FRAMEWORK_END
#define NAMESPACE_FRAMEWORK_END }
#endif

#ifndef USE_NAMESPACE_FRAMEWORK
#define USE_NAMESPACE_FRAMEWORK using namespace FRAMEWORK;
#endif

#ifndef NAMESPACE_EXTERNAL_BEGIN
#define NAMESPACE_EXTERNAL_BEGIN namespace EXTERNAL {
#endif

#ifndef NAMESPACE_EXTERNAL_END
#define NAMESPACE_EXTERNAL_END }
#endif

#ifndef USE_NAMESPACE_EXTERNAL
#define USE_NAMESPACE_EXTERNAL using namespace EXTERNAL;
#endif

#ifndef CNEW
#define CNEW new
#endif

#ifndef CDEL
#define CDEL(x) \
    delete (x); \
    x = nullptr;
#endif

#ifndef CNEWARR
#define CNEWARR(T, n) new T[n]
#endif

#ifndef CDELARR
#define CDELARR(x) \
    delete [] x; \
    x = nullptr;
#endif

#ifndef DISABLE_CLASS_COPYABLE
#define DISABLE_CLASS_COPYABLE(x)    \
private:                             \
    x(const x&) = delete;            \
    x& operator=(const x&) = delete; \
    x(const x&&) = delete;


#endif