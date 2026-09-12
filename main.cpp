#include <gpiod.hpp>
#include <chrono>
#include <csignal>
#include <exception>
#include <iostream>
#include <thread>

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

    constexpr unsigned int ledPin = 17;
    constexpr unsigned int buttonPin = 27;

    std::signal(SIGINT, handleInterrupt);

    try {
        gpiod::chip chip{"/dev/gpiochip0"};

        auto request = chip.prepare_request()
            .set_consumer("pi-panel")
            .add_line_settings(
                ledPin,
                gpiod::line_settings{}
                    .set_direction(gpiod::line::direction::OUTPUT)
                    .set_output_value(gpiod::line::value::INACTIVE))
            .add_line_settings(
                buttonPin,
                gpiod::line_settings{}
                    .set_direction(gpiod::line::direction::INPUT)
                    .set_bias(gpiod::line::bias::PULL_UP))
            .do_request();

            // With a pull-up, pressing the button connects the pin
        // to ground, so a low reading means "pressed".
        auto isPressed = [&request]()
        {
            return request.get_value(buttonPin)
                == gpiod::line::value::INACTIVE;
        };

         bool ledOn = false;
        bool lastReading = isPressed();
        bool stablePressed = lastReading;
        auto lastChange = Clock::now();

        std::cout << "Press the button to toggle the LED.\n"
                  << "Press Ctrl+C to exit.\n";

                  while (!stopRequested)
        {
            const bool reading = isPressed();
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
                    ledOn = !ledOn;

                    request.set_value(
                        ledPin,
                        ledOn ? gpiod::line::value::ACTIVE
                              : gpiod::line::value::INACTIVE);

                    std::cout << "LED: "
                              << (ledOn ? "ON" : "OFF")
                              << std::endl;
                }
            }

            std::this_thread::sleep_for(5ms);
        }

        request.set_value(ledPin, gpiod::line::value::INACTIVE);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

      
}