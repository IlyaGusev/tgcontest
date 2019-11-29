#pragma once

#include <chrono>

template <
    typename Clock = std::chrono::high_resolution_clock,
    typename Duration = std::chrono::microseconds>
class Timer {
public:
    Timer()
        : Begin(Clock::now())
    {
    }

    void Reset() {
        Begin = Clock::now();
    }

    double Elapsed() const {
        return std::chrono::duration_cast<Duration>(Clock::now() - Begin).count();
    }

private:
    std::chrono::time_point<Clock> Begin;
};
