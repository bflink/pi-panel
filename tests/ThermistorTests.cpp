#include <gtest/gtest.h>

#include "Controllers/Thermistor.h"

TEST(ThermistorTest, MidpointVoltageEqualsNominalTemperature)
{
    const Thermistor thermistor;

    EXPECT_NEAR(thermistor.temperatureFahrenheit(1.65), 77.0, 0.01);
}

TEST(ThermistorTest, LowerVoltageIsHotterForThermistorConnectedToGround)
{
    const Thermistor thermistor;

    EXPECT_GT(thermistor.temperatureFahrenheit(1.0),
              thermistor.temperatureFahrenheit(2.3));
}

TEST(ThermistorTest, HigherVoltageIsHotterForThermistorConnectedToSupply)
{
    Thermistor::Configuration configuration;
    configuration.thermistorToGround = false;
    const Thermistor thermistor{configuration};

    EXPECT_GT(thermistor.temperatureFahrenheit(2.3),
              thermistor.temperatureFahrenheit(1.0));
}

TEST(ThermistorTest, RejectsVoltageAtDividerLimits)
{
    const Thermistor thermistor;

    EXPECT_THROW(thermistor.temperatureFahrenheit(0.0), std::out_of_range);
    EXPECT_THROW(thermistor.temperatureFahrenheit(3.3), std::out_of_range);
}

TEST(ThermistorTest, ObservedRoomVoltageProducesRoomTemperature)
{
    const Thermistor thermistor;

    EXPECT_NEAR(thermistor.temperatureFahrenheit(1.7), 74.6, 0.1);
}