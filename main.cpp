#include <QCoreApplication>
#include <QObject>
#include <QTimer>

#include <chrono>
#include <csignal>
#include <exception>
#include <iostream>

#include "Controllers/LedController.h"
#include "Controllers/ButtonDebouncher.h"

namespace
{
    volatile std::sig_atomic_t stopRequested = 0;

    void handleInterrupt(int)
    {
        stopRequested = 1;
    }
}

int main(int argc, char* argv[])
{
    using namespace std::chrono_literals;
    using Clock = ButtonDebouncer::Clock;

    QCoreApplication application(argc, argv);

    std::signal(SIGINT, handleInterrupt);
    std::signal(SIGTERM, handleInterrupt);
    std::signal(SIGHUP, handleInterrupt);

    try
    {
        LedController ledController;

        ButtonDebouncer buttonDebouncer{
            ledController.isButtonPressed(),
            Clock::now()};
            
        QTimer pollTimer;
        pollTimer.setInterval(5ms);
        pollTimer.setTimerType(Qt::PreciseTimer);

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

                // Handle errors here so exceptions don't escape
                // through Qt's event-dispatch code.
                try
                {
                    const bool pressed =
                        ledController.isButtonPressed();

                    if (buttonDebouncer.update(pressed, Clock::now()))
                    {
                        ledController.toggle();

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

        std::cout << "Press the button to toggle the LED.\n"
                  << "Press Ctrl+C to exit.\n";

        pollTimer.start();

        const int exitCode = application.exec();

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