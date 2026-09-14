#pragma once
#include "IGpioService.h"
#include <memory>
#include <string>

class LedController
{
public:
    explicit LedController(unsigned int ledPin = 17,
                           unsigned int buttonPin = 23,
                           const std::string& chipPath = "/dev/gpiochip0");

    explicit LedController(std::shared_ptr<IGpioService> gpioService,
                           unsigned int ledPin = 17,
                           unsigned int buttonPin = 27);

    ~LedController();

    bool isOn() const;
    bool isButtonPressed() const;
    void turnOff();
    void toggle();

private:
    unsigned int m_ledPin;
    unsigned int m_buttonPin;
    bool m_ledOn = false;
    std::shared_ptr<IGpioService> m_gpioService;
};   