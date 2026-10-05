#pragma once

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

namespace brotest {

inline int& failures() {
    static int n = 0;
    return n;
}

inline void fail(const char* file, int line, const std::string& what) {
    ++failures();
    std::fprintf(stderr, "FAIL %s:%d: %s\n", file, line, what.c_str());
    std::fflush(stderr);
}

template <class A, class B>
std::string describe(const char* ea, const char* eb, const A& a, const B& b) {
    std::ostringstream s;
    s << ea << " == " << eb << " (got " << a << " vs " << b << ")";
    return s.str();
}

inline int finish(const char* name) {
    if (failures() == 0) {
        std::printf("[%s] ALL CHECKS PASSED\n", name);
        return 0;
    }
    std::printf("[%s] FAILED (%d check%s)\n", name, failures(), failures() == 1 ? "" : "s");
    return 1;
}

} // namespace brotest

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) ::brotest::fail(__FILE__, __LINE__, #cond);       \
    } while (0)

#define CHECK_EQ(a, b)                                                                 \
    do {                                                                               \
        auto check_a_ = (a);                                                           \
        auto check_b_ = (b);                                                           \
        if (!(check_a_ == check_b_))                                                   \
            ::brotest::fail(__FILE__, __LINE__,                                        \
                           ::brotest::describe(#a, #b, check_a_, check_b_));           \
    } while (0)

#define CHECK_NEAR(a, b, eps)                                                          \
    do {                                                                               \
        double check_a_ = static_cast<double>(a);                                      \
        double check_b_ = static_cast<double>(b);                                      \
        double check_diff_ = std::abs(check_a_ - check_b_);                            \
        if (check_diff_ > static_cast<double>(eps)) {                                  \
            std::ostringstream s_;                                                     \
            s_ << #a << " ~= " << #b << " (got " << check_a_ << " vs " << check_b_     \
               << ", diff " << check_diff_ << " > " << eps << ")";                     \
            ::brotest::fail(__FILE__, __LINE__, s_.str());                             \
        }                                                                              \
    } while (0)

#define REQUIRE(cond)                                                  \
    do {                                                               \
        if (!(cond)) {                                                 \
            ::brotest::fail(__FILE__, __LINE__, "required: " #cond);   \
            return 1;                                                  \
        }                                                              \
    } while (0)

