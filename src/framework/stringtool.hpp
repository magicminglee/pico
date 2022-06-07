#pragma once

#include "event2/event.h"

#include "common.hpp"
#include "contex.hpp"
#include "object.hpp"

NAMESPACE_FRAMEWORK_BEGIN

class CStringTool : public CObject {
public:
    static auto Split(std::string_view sv, std::string_view sp)
    {
        std::vector<std::string_view> res;
        for (size_t b = 0, e = 0; e != std::string_view::npos;) {
            e = sv.find(sp, b);
            res.push_back(std::move(std::string_view(sv.data() + b, e == std::string_view::npos ? sv.size() - b : e - b)));
            b = e + sp.size();
        }
        return res;
    }

    template <typename T>
    static T ToInteger(std::string_view sv)
    {
        T v;
        std::from_chars(sv.data(), sv.data() + sv.size(), v);
        return v;
    }

    static bool IsEqual(const std::string_view& src, const std::string_view& other)
    {
        return src == other;
    }

    static bool IsLess(const std::string_view& src, const std::string_view& other)
    {
        return src < other;
    }

    static bool IsLarge(const std::string_view& src, const std::string_view& other)
    {
        return src > other;
    }

    static void Trim(std::string& src, std::string_view sp)
    {
        std::string_view::value_type buf[512];
        size_t len = 0;
        if (src.size() > sizeof(buf))
            return;
        std::vector<std::string_view> res;
        std::string_view sv = src;
        for (size_t b = 0, e = 0; e != std::string_view::npos;) {
            e = sv.find(sp, b);
            res.push_back(std::move(std::string_view(sv.data() + b, e == std::string_view::npos ? sv.size() - b : e - b)));
            b = e + sp.size();
        }
        for (auto& v : res) {
            v.copy(buf + len, v.size(), 0);
            len += v.size();
        }
        src = std::move(std::string(buf, len));
    }

    template <class... Args>
    static std::string Format(std::string_view fmt, const Args&... args)
    {
        char buf[1024];
        size_t size = snprintf(buf, sizeof(buf), fmt.data(), args...);
        if (size <= 0 || size > sizeof(buf)) {
            throw std::runtime_error("Error during formatting.");
        }
        return std::move(std::string(buf, size));
    }

    static std::string ToUpper(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return std::move(str);
    }

    static std::string ToLower(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return std::move(str);
    }

    DISABLE_CLASS_COPYABLE(CStringTool);
};

NAMESPACE_FRAMEWORK_END