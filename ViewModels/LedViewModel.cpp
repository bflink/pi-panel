#include "LedViewModel.h"
#include "../Controllers/Ads1015.h"
#include "../Controllers/LedController.h"
#include "../Controllers/SystemTemperatureReader.h"
#include "../Controllers/Thermistor.h"

#include <exception>
#include <QVariantMap>

LedViewModel::LedViewModel(
    LedController& ledController,
    Ads1015& adc,
    Thermistor& thermistor,
    SystemTemperatureReader& systemTemperatureReader,
    QObject* parent)
    : QObject(parent),
      m_ledController(ledController),
      m_adc(adc),
    m_thermistor(thermistor),
    m_systemTemperatureReader(systemTemperatureReader)
{
}

bool LedViewModel::isLedOn() const
{
    return m_ledController.isOn();
}

double LedViewModel::analogVoltage() const
{
    return m_analogVoltage;
}

bool LedViewModel::isAnalogAvailable() const
{
    return m_analogAvailable;
}

QString LedViewModel::analogError() const
{
    return m_analogError;
}

double LedViewModel::temperatureFahrenheit() const
{
    return m_temperatureFahrenheit;
}

bool LedViewModel::isTemperatureAvailable() const
{
    return m_temperatureAvailable;
}

QString LedViewModel::temperatureError() const
{
    return m_temperatureError;
}

QVariantList LedViewModel::systemTemperatures() const
{
    return m_systemTemperatures;
}

void LedViewModel::toggle()
{
    try
    {
        m_ledController.toggle();
    }
    catch (const std::exception& error)
    {
        emit operationFailed(QString::fromUtf8(error.what()));
        return;
    }

    emit ledOnChanged();
}

void LedViewModel::sampleAnalogInput()
{
    try
    {
        m_analogVoltage = m_adc.readVoltage(3);
        emit analogVoltageChanged();

        if (!m_analogAvailable || !m_analogError.isEmpty())
        {
            m_analogAvailable = true;
            m_analogError.clear();
            emit analogStatusChanged();
        }
    }
    catch (const std::exception& error)
    {
        const QString message = QString::fromUtf8(error.what());
        if (m_analogAvailable || m_analogError != message)
        {
            m_analogAvailable = false;
            m_analogError = message;
            emit analogStatusChanged();
        }
        return;
    }

    try
    {
        m_temperatureFahrenheit =
            m_thermistor.temperatureFahrenheit(m_analogVoltage);
        emit temperatureChanged();

        if (!m_temperatureAvailable || !m_temperatureError.isEmpty())
        {
            m_temperatureAvailable = true;
            m_temperatureError.clear();
            emit temperatureStatusChanged();
        }
    }
    catch (const std::exception& error)
    {
        const QString message = QString::fromUtf8(error.what());
        if (m_temperatureAvailable || m_temperatureError != message)
        {
            m_temperatureAvailable = false;
            m_temperatureError = message;
            emit temperatureStatusChanged();
        }
    }
}

void LedViewModel::sampleSystemTemperatures()
{
    QVariantList temperatures;
    for (const auto& reading :
         m_systemTemperatureReader.readTemperatures())
    {
        temperatures.append(QVariantMap{
            {QStringLiteral("name"),
             QString::fromStdString(reading.name)},
            {QStringLiteral("fahrenheit"), reading.fahrenheit}});
    }

    m_systemTemperatures = std::move(temperatures);
    emit systemTemperaturesChanged();
}