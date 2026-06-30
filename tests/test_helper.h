#pragma once

#include <PoolPro/Pool.h>

#include <iostream>
#include <string>

// global test counters
inline int total  = 0;
inline int pass   = 0;
inline int fail   = 0;

// terminal colors
constexpr const char* RESET = "\033[0m";
constexpr const char* GREEN = "\033[92m";
constexpr const char* RED   = "\033[91m";
constexpr const char* CYAN  = "\033[96m";
constexpr const char* GRAY  = "\033[37m";

// prints a horizontal separator line
inline void borderLine() {
    std::cout << GRAY << std::string(70, '-') << RESET << "\n";
}

// prints the overall test statistics
inline void stats() {
    std::cout << "T: " << total << "\n";
    std::cout << "P: " << pass << "\n";
    std::cout << "F: " << fail << "\n";
}

// prints a section title in cyan
inline void setTitle(std::string_view title) {
    std::cout << CYAN << title << RESET << "\n";
}

// converts snake_case function name to Title Case for display
inline std::string prettify(const char* name) {
    std::string result{name};
    bool firstLetter = true;

    for (char& c : result) {
        if (firstLetter) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            firstLetter = false;
        } else {
            if (c == '_') {
                c = ' ';
                firstLetter = true;
            }
        }
    }
    return result;
}

// runs a test function and prints PASS on success
#define RUN(name) do {                  \
    name();                             \
    std::cout << GREEN                  \
              << "[PASS] "              \
              << RESET                  \
              << prettify(#name)        \
              << "\n";                  \
} while (0)

// checks an expression and tracks pass/fail/total counts
#define CHK(expr) do {                  \
    if (!(expr)) {                      \
        ++fail;                         \
        std::cout << RED                \
                  << "[FAIL] "          \
                  << RESET              \
                  << "("                \
                  << __FILE__           \
                  << ":"                \
                  << __LINE__           \
                  << ")\n";             \
    } else ++pass;                      \
    ++total;                            \
} while (0)

// Shared Callable
struct Tracker {
    inline static int constructions = 0;
    inline static int destructions  = 0;

    std::uintptr_t value = 0;

    explicit Tracker(std::uintptr_t v = 0) : value(v) {
        ++constructions;
    }

    ~Tracker() {
        ++destructions;
    }

    Tracker(const Tracker&)            = delete;
    Tracker& operator=(const Tracker&) = delete;

    static void reset() {
        constructions = 0;
        destructions  = 0;
    }
};

static_assert(sizeof(Tracker) >= sizeof(void*));
