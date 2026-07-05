#pragma once

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