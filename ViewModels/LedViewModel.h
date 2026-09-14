#pragma once

#include <QObject>
#include <QString>

class LedController;
class Ads1015;
class Thermistor;

class LedViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool ledOn READ isLedOn NOTIFY ledOnChanged)
    Q_PROPERTY(double analogVoltage READ analogVoltage NOTIFY analogVoltageChanged)
    Q_PROPERTY(bool analogAvailable READ isAnalogAvailable NOTIFY analogStatusChanged)
    Q_PROPERTY(QString analogError READ analogError NOTIFY analogStatusChanged)
    Q_PROPERTY(double temperatureFahrenheit READ temperatureFahrenheit NOTIFY temperatureChanged)
    Q_PROPERTY(bool temperatureAvailable READ isTemperatureAvailable NOTIFY temperatureStatusChanged)
    Q_PROPERTY(QString temperatureError READ temperatureError NOTIFY temperatureStatusChanged)

public:
    explicit LedViewModel(
        LedController& ledController,
        Ads1015& adc,
        Thermistor& thermistor,
        QObject* parent = nullptr);

    bool isLedOn() const;
    double analogVoltage() const;
    bool isAnalogAvailable() const;
    QString analogError() const;
    double temperatureFahrenheit() const;
    bool isTemperatureAvailable() const;
    QString temperatureError() const;

    Q_INVOKABLE void toggle();
    void sampleAnalogInput();

signals:
    void ledOnChanged();
    void analogVoltageChanged();
    void analogStatusChanged();
    void temperatureChanged();
    void temperatureStatusChanged();
    void operationFailed(const QString& message);

private:
    LedController& m_ledController;
    Ads1015& m_adc;
    Thermistor& m_thermistor;
    double m_analogVoltage = 0.0;
    bool m_analogAvailable = false;
    QString m_analogError;
    double m_temperatureFahrenheit = 0.0;
    bool m_temperatureAvailable = false;
    QString m_temperatureError;
};