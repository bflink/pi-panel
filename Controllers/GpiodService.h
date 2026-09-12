#pragma once
#include "IGpioService.h"
#include <gpiod.hpp>
#include <string>

class GpiodService : public IGpioService
{
public:
    GpiodService(unsigned int ledPin,
                 unsigned int buttonPin,
                 const std::string& chipPath = "/dev/gpiochip0");

    void setLineValue(unsigned int pin, gpiod::line::value value) override;
    gpiod::line::value getLineValue(unsigned int pin) const override;

private:
    static gpiod::line_request createRequest(unsigned int ledPin,
                                             unsigned int buttonPin,
                                             const std::string& chipPath);

    mutable gpiod::line_request m_request;
};
