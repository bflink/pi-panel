#include "GpiodService.h"
#include <iostream>

gpiod::line_request GpiodService::createRequest(unsigned int ledPin,
                                                unsigned int buttonPin,
                                                const std::string& chipPath)
{
    try
    {
        gpiod::chip chip{chipPath};
        return chip.prepare_request()
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
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error creating GPIO request: " << error.what() << '\n';
        throw;
    }
}

GpiodService::GpiodService(unsigned int ledPin,
                           unsigned int buttonPin,
                           const std::string& chipPath)
    : m_request(createRequest(ledPin, buttonPin, chipPath))
{
}

void GpiodService::setLineValue(unsigned int pin, gpiod::line::value value)
{
    m_request.set_value(pin, value);
}

gpiod::line::value GpiodService::getLineValue(unsigned int pin) const
{
    return m_request.get_value(pin);
}
