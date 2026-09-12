#pragma once

#include <chrono>

class ButtonDebouncer
{
public:
    using Clock = std::chrono::steady_clock;

    explicit ButtonDebouncer(
        bool initiallyPressed,
        Clock::time_point now,
        std::chrono::milliseconds debounceInterval = std::chrono::milliseconds(30));

    bool update(bool pressed, Clock::time_point now);

private:
    std::chrono::milliseconds m_debounceInterval;
    bool m_lastReading;
    bool m_stableState;
    Clock::time_point m_lastChange;
};