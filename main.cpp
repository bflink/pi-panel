#include <chrono>
#include <csignal>
#include <exception>
#include <iostream>
#include <thread>
#include "Controllers/LedController.h"

namespace
{
    volatile std::sig_atomic_t stopRequested = 0;

    void handleInterrupt(int)
    {
        stopRequested = 1;
    }
}

int main()
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::steady_clock;

    std::signal(SIGINT, handleInterrupt);
    std::signal(SIGTERM, handleInterrupt);
    std::signal(SIGHUP, handleInterrupt);

    try
    {
        LedController ledController;

        bool lastReading = ledController.isButtonPressed();
        bool stablePressed = lastReading;
        auto lastChange = Clock::now();

        std::cout << "Press the button to toggle the LED.\n"
                  << "Press Ctrl+C to exit.\n";

        while (!stopRequested)
        {
            const bool reading = ledController.isButtonPressed();
            const auto now = Clock::now();

            // Restart the debounce interval on every raw change.
            if (reading != lastReading)
            {
                lastReading = reading;
                lastChange = now;
            }

            // Accept the new state after 30 ms without changes.
            if (reading != stablePressed &&
                now - lastChange >= 30ms)
            {
                stablePressed = reading;

                // Toggle on a press, but not on a release.
                if (stablePressed)
                {
                    ledController.toggle();

                    std::cout << "LED: "
                              << (ledController.isOn() ? "ON" : "OFF")
                              << '\n';
                }
            }

            std::this_thread::sleep_for(5ms);
        }

        ledController.turnOff();
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}