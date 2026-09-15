#pragma once

#include <string>
#include <vector>

struct SystemTemperature
{
    std::string name;
    double fahrenheit;
};

class SystemTemperatureReader
{
public:
    explicit SystemTemperatureReader(
        std::string hwmonPath = "/sys/class/hwmon");

    std::vector<SystemTemperature> readTemperatures() const;

private:
    std::string m_hwmonPath;
};