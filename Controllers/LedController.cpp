#include "LedController.h"
#include <iostream>

namespace
{
    gpiod::line_request createRequest(unsigned int ledPin,
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
            std::cerr << "Error: " << error.what() << '\n';
            throw;
        }
    }
}

LedController::LedController(unsigned int ledPin,
                             unsigned int buttonPin,
                             const std::string& chipPath)
    : m_ledPin(ledPin),
      m_buttonPin(buttonPin),
      m_ledOn(false),
      m_request(createRequest(ledPin, buttonPin, chipPath))
{
}

LedController::~LedController()
{
    try
    {
        turnOff();
    }
    catch (...)
    {
        // Suppress exceptions in destructor
    }
}

bool LedController::isOn() const
{
    return m_ledOn;
}

bool LedController::isButtonPressed() const
{
    return m_request.get_value(m_buttonPin) == gpiod::line::value::INACTIVE;
}

void LedController::turnOff()
{
    m_request.set_value(m_ledPin, gpiod::line::value::INACTIVE);
    m_ledOn = false;
}

void LedController::toggle()
{
    m_request.set_value(m_ledPin, m_ledOn ? gpiod::line::value::INACTIVE : gpiod::line::value::ACTIVE);
    m_ledOn = !m_ledOn;
}
