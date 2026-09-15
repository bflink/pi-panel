#include "Networking/TelemetryClient.h"
#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: telemetry_probe HOST:PORT\n";
        return 2;
    }
    try
    {
        TelemetryClient client(argv[1]);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (std::chrono::steady_clock::now() < deadline)
        {
            const auto s = client.snapshot();
            if (s.readings[0].available && s.readings[1].available && s.readings[2].available)
            {
                std::cout << "Connected to " << argv[1] << '\n'
                          << "Temperature: " << s.readings[0].value << " C\n"
                          << "Pressure: " << s.readings[1].value << " kPa\n"
                          << "Waveform: " << s.readings[2].value << " V\n";
                return 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        std::cerr << "No complete telemetry connection within 10 seconds.\n";
        const auto s = client.snapshot();
        const char* names[] = {"Temperature", "Pressure", "Waveform"};
        for (std::size_t i = 0; i < TelemetryClient::StreamCount; ++i)
            std::cerr << names[i] << ": " << s.readings[i].status << '\n';
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
