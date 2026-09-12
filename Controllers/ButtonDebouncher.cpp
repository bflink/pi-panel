#include "ButtonDebouncher.h"

ButtonDebouncer::ButtonDebouncer(bool initiallyPressed, std::chrono::steady_clock::time_point now, std::chrono::milliseconds debounceInterval)
    : m_debounceInterval(debounceInterval),
      m_lastReading(initiallyPressed),
      m_stableState(initiallyPressed),
      m_lastChange(now)
{
}

bool ButtonDebouncer::update(bool pressed, Clock::time_point now)
{
    // Restart the debounce interval on every raw change.
    if (pressed != m_lastReading)
    {
        m_lastReading = pressed;
        m_lastChange = now;
    }

    //nothing to report if the accepted state is unchanged
    if (pressed == m_stableState)
    {
        return false;
    }

    // The new reading must remain unchanged long enough.
    if (now - m_lastChange < m_debounceInterval)
    {
        return false;
    }

    m_stableState = pressed;
    return m_stableState;
}
