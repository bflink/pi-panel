#include <gtest/gtest.h>
#include <chrono>
#include "Controllers/ButtonDebouncher.h"

using namespace std::chrono_literals;

class ButtonDebouncerTest : public ::testing::Test
{
protected:
    using Clock = std::chrono::steady_clock;
    Clock::time_point m_startTime = Clock::time_point{0s};
    const std::chrono::milliseconds m_debounceInterval{30ms};
};

TEST_F(ButtonDebouncerTest, InitiallyUnpressedNoEvent)
{
    ButtonDebouncer debouncer(false, m_startTime, m_debounceInterval);

    // Reading remains unpressed
    EXPECT_FALSE(debouncer.update(false, m_startTime + 10ms));
    EXPECT_FALSE(debouncer.update(false, m_startTime + 50ms));
}

TEST_F(ButtonDebouncerTest, PressIgnoredBeforeDebounceInterval)
{
    ButtonDebouncer debouncer(false, m_startTime, m_debounceInterval);

    // Button pressed, but not enough time elapsed
    EXPECT_FALSE(debouncer.update(true, m_startTime + 5ms));
    EXPECT_FALSE(debouncer.update(true, m_startTime + 15ms));
    EXPECT_FALSE(debouncer.update(true, m_startTime + 29ms));
}

TEST_F(ButtonDebouncerTest, StablePressTriggersEvent)
{
    ButtonDebouncer debouncer(false, m_startTime, m_debounceInterval);

    // Press starts
    debouncer.update(true, m_startTime + 10ms);

    // Time reaches 30ms after change (10ms + 30ms = 40ms)
    EXPECT_TRUE(debouncer.update(true, m_startTime + 40ms));

    // Subsequent updates with same pressed state should not re-trigger
    EXPECT_FALSE(debouncer.update(true, m_startTime + 45ms));
    EXPECT_FALSE(debouncer.update(true, m_startTime + 100ms));
}

TEST_F(ButtonDebouncerTest, BouncingResetsTimer)
{
    ButtonDebouncer debouncer(false, m_startTime, m_debounceInterval);

    // Initial press at 10ms
    debouncer.update(true, m_startTime + 10ms);
    // Bounce back to false at 20ms
    debouncer.update(false, m_startTime + 20ms);
    // Press again at 30ms
    debouncer.update(true, m_startTime + 30ms);

    // At 50ms (only 20ms since last change at 30ms) -> should NOT trigger
    EXPECT_FALSE(debouncer.update(true, m_startTime + 50ms));

    // At 60ms (30ms since change at 30ms) -> triggers!
    EXPECT_TRUE(debouncer.update(true, m_startTime + 60ms));
}

TEST_F(ButtonDebouncerTest, ReleaseDoesNotTriggerPressAction)
{
    ButtonDebouncer debouncer(false, m_startTime, m_debounceInterval);

    // Press and stabilize
    debouncer.update(true, m_startTime);
    EXPECT_TRUE(debouncer.update(true, m_startTime + 30ms));

    // Release button at 100ms
    debouncer.update(false, m_startTime + 100ms);

    // After 30ms of release (130ms), update returns false (not a press event)
    EXPECT_FALSE(debouncer.update(false, m_startTime + 130ms));
}

TEST_F(ButtonDebouncerTest, MultiplePressAndReleaseCycles)
{
    ButtonDebouncer debouncer(false, m_startTime, m_debounceInterval);

    // 1st press
    debouncer.update(true, m_startTime + 10ms);
    EXPECT_TRUE(debouncer.update(true, m_startTime + 40ms));

    // 1st release
    debouncer.update(false, m_startTime + 60ms);
    EXPECT_FALSE(debouncer.update(false, m_startTime + 90ms));

    // 2nd press
    debouncer.update(true, m_startTime + 120ms);
    EXPECT_TRUE(debouncer.update(true, m_startTime + 150ms));
}
