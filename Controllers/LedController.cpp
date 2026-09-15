#include "LedController.h"
#include "GpiodService.h"

LedController::LedController(unsigned int ledPin,
                             unsigned int buttonPin,
                             const std::string &chipPath)
    : m_ledPin(ledPin),
      m_buttonPin(buttonPin),
      m_ledOn(false),
      m_gpioService(std::make_shared<GpiodService>(ledPin, buttonPin, chipPath))
{
}

LedController::LedController(std::shared_ptr<IGpioService> gpioService,
                             unsigned int ledPin,
                             unsigned int buttonPin)
    : m_ledPin(ledPin),
      m_buttonPin(buttonPin),
      m_ledOn(false),
      m_gpioService(std::move(gpioService))
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
    if (!m_gpioService)
    {
        return false;
    }
    return m_gpioService->getLineValue(m_buttonPin) == gpiod::line::value::ACTIVE;
}

void LedController::turnOff()
{
    if (m_gpioService)
    {
        m_gpioService->setLineValue(m_ledPin, gpiod::line::value::INACTIVE);
    }
    m_ledOn = false;
}

void LedController::toggle()
{
    if (m_gpioService)
    {
        m_gpioService->setLineValue(m_ledPin, m_ledOn ? gpiod::line::value::INACTIVE : gpiod::line::value::ACTIVE);
    }
    m_ledOn = !m_ledOn;
}
