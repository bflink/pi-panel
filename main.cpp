#include <gpiod.hpp>
#include <exception>
#include <iostream>

int main()
{
    try
    {
        constexpr unsigned int ledPin = 17;

        gpiod::chip chip{"/dev/gpiochip0"};

        auto request = chip.prepare_request()
            .set_consumer("pi-panel")
            .add_line_settings(
                ledPin,
                gpiod::line_settings{}
                    .set_direction(gpiod::line::direction::OUTPUT)
                    .set_output_value(gpiod::line::value::INACTIVE))
            .do_request();

        bool ledOn = false;
        char command{};

        std::cout << "t = toggle LED, q = quit\n";

        while (std::cin >> command)
        {
            if (command == 'q')
                break;

            if (command == 't')
            {
                ledOn = !ledOn;

                request.set_value(
                    ledPin,
                    ledOn ? gpiod::line::value::ACTIVE
                          : gpiod::line::value::INACTIVE);

                std::cout << "LED: " << (ledOn ? "ON" : "OFF") << '\n';
            }
        }

        request.set_value(ledPin, gpiod::line::value::INACTIVE);
    }
    catch (const std::exception& error)
    {
        std::cerr << "GPIO error: " << error.what() << '\n';
        return 1;
    }
}