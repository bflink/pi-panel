#pragma once

class Thermistor
{
public:
    struct Configuration
    {
        double supplyVoltage = 3.3;
        double fixedResistanceOhms = 10000.0;
        double nominalResistanceOhms = 10000.0;
        double nominalTemperatureCelsius = 25.0;
        double betaKelvin = 3950.0;
        bool thermistorToGround = true;
    };

    Thermistor();
    explicit Thermistor(Configuration configuration);

    double temperatureFahrenheit(double voltage) const;

private:
    Configuration m_configuration;
};