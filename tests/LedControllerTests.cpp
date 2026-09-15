#include <gtest/gtest.h>
#include <map>
#include <vector>
#include "Controllers/LedController.h"
#include "Controllers/IGpioService.h"

namespace
{
    class MockGpioService : public IGpioService
    {
    public:
        struct SetValueCall
        {
            unsigned int pin;
            gpiod::line::value value;
        };

        void setLineValue(unsigned int pin, gpiod::line::value value) override
        {
            m_setCalls.push_back({pin, value});
            m_pinValues[pin] = value;
        }

        gpiod::line::value getLineValue(unsigned int pin) const override
        {
            auto it = m_pinValues.find(pin);
            if (it != m_pinValues.end())
            {
                return it->second;
            }
            return gpiod::line::value::ACTIVE;
        }

        void setPinValue(unsigned int pin, gpiod::line::value value)
        {
            m_pinValues[pin] = value;
        }

        const std::vector<SetValueCall>& getSetCalls() const
        {
            return m_setCalls;
        }

        void clearSetCalls()
        {
            m_setCalls.clear();
        }

    private:
        std::map<unsigned int, gpiod::line::value> m_pinValues;
        std::vector<SetValueCall> m_setCalls;
    };
}

class LedControllerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_mockGpio = std::make_shared<MockGpioService>();
        m_ledPin = 17;
        m_buttonPin = 27;
        m_controller = std::make_unique<LedController>(m_mockGpio, m_ledPin, m_buttonPin);
    }

    std::shared_ptr<MockGpioService> m_mockGpio;
    unsigned int m_ledPin;
    unsigned int m_buttonPin;
    std::unique_ptr<LedController> m_controller;
};

TEST_F(LedControllerTest, InitiallyOff)
{
    EXPECT_FALSE(m_controller->isOn());
}

TEST_F(LedControllerTest, ToggleTurnsOn)
{
    m_controller->toggle();

    EXPECT_TRUE(m_controller->isOn());
    ASSERT_EQ(m_mockGpio->getSetCalls().size(), 1u);
    EXPECT_EQ(m_mockGpio->getSetCalls().back().pin, m_ledPin);
    EXPECT_EQ(m_mockGpio->getSetCalls().back().value, gpiod::line::value::ACTIVE);
}

TEST_F(LedControllerTest, ToggleTwiceTurnsOff)
{
    m_controller->toggle();
    m_controller->toggle();

    EXPECT_FALSE(m_controller->isOn());
    ASSERT_EQ(m_mockGpio->getSetCalls().size(), 2u);
    EXPECT_EQ(m_mockGpio->getSetCalls().back().pin, m_ledPin);
    EXPECT_EQ(m_mockGpio->getSetCalls().back().value, gpiod::line::value::INACTIVE);
}

TEST_F(LedControllerTest, TurnOffWhenOn)
{
    m_controller->toggle();
    EXPECT_TRUE(m_controller->isOn());
    m_mockGpio->clearSetCalls();

    m_controller->turnOff();

    EXPECT_FALSE(m_controller->isOn());
    ASSERT_EQ(m_mockGpio->getSetCalls().size(), 1u);
    EXPECT_EQ(m_mockGpio->getSetCalls().back().pin, m_ledPin);
    EXPECT_EQ(m_mockGpio->getSetCalls().back().value, gpiod::line::value::INACTIVE);
}

TEST_F(LedControllerTest, TurnOffWhenAlreadyOff)
{
    m_controller->turnOff();

    EXPECT_FALSE(m_controller->isOn());
    ASSERT_EQ(m_mockGpio->getSetCalls().size(), 1u);
    EXPECT_EQ(m_mockGpio->getSetCalls().back().pin, m_ledPin);
    EXPECT_EQ(m_mockGpio->getSetCalls().back().value, gpiod::line::value::INACTIVE);
}

TEST_F(LedControllerTest, DestructorTurnsOffLed)
{
    m_controller->toggle();
    EXPECT_TRUE(m_controller->isOn());
    m_mockGpio->clearSetCalls();

    // Destroy controller
    m_controller.reset();

    ASSERT_EQ(m_mockGpio->getSetCalls().size(), 1u);
    EXPECT_EQ(m_mockGpio->getSetCalls().back().pin, m_ledPin);
    EXPECT_EQ(m_mockGpio->getSetCalls().back().value, gpiod::line::value::INACTIVE);
}

TEST_F(LedControllerTest, IsButtonPressedWhenHigh)
{
    m_mockGpio->setPinValue(m_buttonPin, gpiod::line::value::ACTIVE);
    EXPECT_TRUE(m_controller->isButtonPressed());
}

TEST_F(LedControllerTest, IsButtonReleasedWhenLow)
{
    m_mockGpio->setPinValue(m_buttonPin, gpiod::line::value::INACTIVE);
    EXPECT_FALSE(m_controller->isButtonPressed());
}

TEST_F(LedControllerTest, CustomPinsAreUsed)
{
    auto customMock = std::make_shared<MockGpioService>();
    const unsigned int customLedPin = 22;
    const unsigned int customButtonPin = 23;
    LedController customController(customMock, customLedPin, customButtonPin);

    customController.toggle();
    ASSERT_EQ(customMock->getSetCalls().size(), 1u);
    EXPECT_EQ(customMock->getSetCalls().back().pin, customLedPin);

    customMock->setPinValue(customButtonPin, gpiod::line::value::ACTIVE);
    EXPECT_TRUE(customController.isButtonPressed());
}
