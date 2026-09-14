#include "Thermistor.h"

#include <cmath>
#include <stdexcept>

Thermistor::Thermistor()
    : Thermistor(Configuration{})
{
}

Thermistor::Thermistor(Configuration configuration)
    : m_configuration(configuration)
{
    if (m_configuration.supplyVoltage <= 0.0 ||
        m_configuration.fixedResistanceOhms <= 0.0 ||
        m_configuration.nominalResistanceOhms <= 0.0 ||
        m_configuration.betaKelvin <= 0.0)
    {
        throw std::invalid_argument("Thermistor configuration values must be positive");
    }
}

double Thermistor::temperatureFahrenheit(double voltage) const
{
    if (voltage <= 0.0 || voltage >= m_configuration.supplyVoltage)
    {
        throw std::out_of_range("Thermistor voltage is outside the divider range");
    }

    const double thermistorResistance = m_configuration.thermistorToGround
        ? m_configuration.fixedResistanceOhms * voltage /
              (m_configuration.supplyVoltage - voltage)
        : m_configuration.fixedResistanceOhms *
              (m_configuration.supplyVoltage - voltage) / voltage;
    const double nominalTemperatureKelvin =
        m_configuration.nominalTemperatureCelsius + 273.15;
    const double inverseTemperatureKelvin =
        (1.0 / nominalTemperatureKelvin) +
        (std::log(thermistorResistance /
                  m_configuration.nominalResistanceOhms) /
         m_configuration.betaKelvin);
    const double temperatureCelsius =
        (1.0 / inverseTemperatureKelvin) - 273.15;

    return temperatureCelsius * 9.0 / 5.0 + 32.0;
}