#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QObject>
#include <QTimer>
#include <QVariant>
#include "ViewModels/LedViewModel.h"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>

#include "Controllers/Ads1015.h"
#include "Controllers/LedController.h"
#include "Controllers/ButtonDebouncher.h"
#include "Controllers/SystemTemperatureReader.h"
#include "Controllers/Thermistor.h"

namespace
{
    volatile std::sig_atomic_t stopRequested = 0;

    std::string adcDevicePath()
    {
        const char* configuredPath = std::getenv("PI_PANEL_I2C_DEVICE");
        return configuredPath != nullptr ? configuredPath : "/dev/i2c-1";
    }

    double configuredDouble(const char* name, double defaultValue)
    {
        const char* value = std::getenv(name);
        if (value == nullptr)
        {
            return defaultValue;
        }

        std::size_t parsedCharacters = 0;
        const double parsedValue = std::stod(value, &parsedCharacters);
        if (value[parsedCharacters] != '\0')
        {
            throw std::invalid_argument(std::string{name} + " must be a number");
        }
        return parsedValue;
    }

    Thermistor::Configuration thermistorConfiguration()
    {
        Thermistor::Configuration configuration;
        configuration.supplyVoltage = configuredDouble(
            "PI_PANEL_DIVIDER_VOLTS", configuration.supplyVoltage);
        configuration.fixedResistanceOhms = configuredDouble(
            "PI_PANEL_FIXED_RESISTOR_OHMS", configuration.fixedResistanceOhms);
        configuration.nominalResistanceOhms = configuredDouble(
            "PI_PANEL_THERMISTOR_NOMINAL_OHMS", configuration.nominalResistanceOhms);
        configuration.nominalTemperatureCelsius = configuredDouble(
            "PI_PANEL_THERMISTOR_NOMINAL_C", configuration.nominalTemperatureCelsius);
        configuration.betaKelvin = configuredDouble(
            "PI_PANEL_THERMISTOR_BETA", configuration.betaKelvin);

        const char* thermistorToGround =
            std::getenv("PI_PANEL_THERMISTOR_TO_GROUND");
        if (thermistorToGround != nullptr)
        {
            configuration.thermistorToGround =
                std::string{thermistorToGround} != "0";
        }
        return configuration;
    }

    void handleInterrupt(int)
    {
        stopRequested = 1;
    }
}

int main(int argc, char* argv[])
{
    using namespace std::chrono_literals;
    using Clock = ButtonDebouncer::Clock;

    QGuiApplication application(argc, argv);

    std::signal(SIGINT, handleInterrupt);
    std::signal(SIGTERM, handleInterrupt);
    std::signal(SIGHUP, handleInterrupt);

    try
    {
        LedController ledController;
        Ads1015 adc{adcDevicePath()};
        Thermistor thermistor{thermistorConfiguration()};
        SystemTemperatureReader systemTemperatureReader;
        LedViewModel ledViewModel{
            ledController,
            adc,
            thermistor,
            systemTemperatureReader};

        ButtonDebouncer buttonDebouncer{
            ledController.isButtonPressed(),
            Clock::now()};
            
        QTimer pollTimer;
        pollTimer.setInterval(5ms);
        pollTimer.setTimerType(Qt::PreciseTimer);

        QTimer analogTimer;
        analogTimer.setInterval(200ms);

        QTimer systemTemperatureTimer;
        systemTemperatureTimer.setInterval(2s);

        QObject::connect(
            &pollTimer,
            &QTimer::timeout,
            &application,
            [&]()
            {
                if (stopRequested)
                {
                    application.quit();
                    return;
                }

                try
                {
                    const bool pressed =
                        ledController.isButtonPressed();

                    if (buttonDebouncer.update(pressed, Clock::now()))
                    {
                        ledViewModel.toggle();

                        std::cout << "LED: "
                                  << (ledController.isOn() ? "ON" : "OFF")
                                  << '\n';
                    }
                }
                catch (const std::exception& error)
                {
                    std::cerr << "GPIO error: "
                              << error.what() << '\n';

                    application.exit(1);
                }
            });

        QObject::connect(
            &analogTimer,
            &QTimer::timeout,
            &ledViewModel,
            &LedViewModel::sampleAnalogInput);

        QObject::connect(
            &systemTemperatureTimer,
            &QTimer::timeout,
            &ledViewModel,
            &LedViewModel::sampleSystemTemperatures);

        std::cout << "Press the button to toggle the LED.\n"
                  << "Press Ctrl+C to exit.\n";

        QQmlApplicationEngine engine;

        engine.setInitialProperties({{QStringLiteral("ledViewModel"),
                                      QVariant::fromValue(&ledViewModel)}});

        engine.load(QUrl{QStringLiteral("qrc:/PiPanel/Main.qml")});

        if (engine.rootObjects().isEmpty())
            return 1;

        ledViewModel.sampleAnalogInput();
        ledViewModel.sampleSystemTemperatures();
        pollTimer.start();
        analogTimer.start();
        systemTemperatureTimer.start();

        const int exitCode = application.exec();

        systemTemperatureTimer.stop();
        analogTimer.stop();
        pollTimer.stop();
        ledController.turnOff();

        return exitCode;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}