#pragma once
#include <gpiod.hpp>
#include <string>

class LedController
{
public:
    explicit LedController(unsigned int ledPin = 17,
                           unsigned int buttonPin = 27,
                           const std::string& chipPath = "/dev/gpiochip0");
    ~LedController();

    bool isOn() const;
    bool isButtonPressed() const;
    void turnOff();
    void toggle();

private:
    unsigned int m_ledPin;
    unsigned int m_buttonPin;
    bool m_ledOn = false;
    mutable gpiod::line_request m_request;
};   