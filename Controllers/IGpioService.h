#pragma once
#include <gpiod.hpp>

class IGpioService
{
public:
    virtual ~IGpioService() = default;
    virtual void setLineValue(unsigned int pin, gpiod::line::value value) = 0;
    virtual gpiod::line::value getLineValue(unsigned int pin) const = 0;
};
