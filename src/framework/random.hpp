#pragma once

#include "common.hpp"

NAMESPACE_FRAMEWORK_BEGIN

//Produces random integer values i, uniformly distributed on the closed interval [0, INT_MAX] by default.
//Otherwise you can specify a uniform_int_distribution distrubuted object.
class CRandomGenerator {
public:
    CRandomGenerator()
        : m_generator(m_rd())
    {
    }

    long Rand(int64_t l, int64_t h)
    {
        int64_t rnd;
        int64_t range = h - l + 1;
        int64_t MAX_RANGE = m_distrib.max();
        int64_t limit = MAX_RANGE - (MAX_RANGE % range);
        do {
            rnd = rand();
        } while (rnd >= limit);
        return (rnd % range) + l;
    }

private:
    std::random_device m_rd;
    std::mt19937 m_generator;
    std::uniform_int_distribution<> m_distrib;
    DISABLE_CLASS_COPYABLE(CRandomGenerator);
};
inline thread_local CRandomGenerator RANDOM;

NAMESPACE_FRAMEWORK_END