#include "SystemTemperatureReader.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>

namespace
{
    std::optional<std::string> readText(const std::filesystem::path& path)
    {
        std::ifstream input{path};
        std::string value;
        if (!(input >> value))
        {
            return std::nullopt;
        }
        return value;
    }

    std::string displayName(const std::string& deviceName,
                            const std::string& sensorLabel)
    {
        if (deviceName == "cpu_thermal")
        {
            return "CPU";
        }
        if (deviceName == "rp1_adc")
        {
            return "RP1 controller";
        }
        if (deviceName == "nvme")
        {
            return sensorLabel == "Composite"
                ? "NVMe"
                : "NVMe " + sensorLabel;
        }
        return sensorLabel.empty()
            ? deviceName
            : deviceName + " " + sensorLabel;
    }

    std::string sensorLabel(const std::filesystem::path& inputPath)
    {
        std::string filename = inputPath.filename().string();
        filename.replace(filename.size() - 6, 6, "label");

        std::ifstream input{inputPath.parent_path() / filename};
        std::string label;
        std::getline(input, label);
        return label;
    }
}

SystemTemperatureReader::SystemTemperatureReader(std::string hwmonPath)
    : m_hwmonPath(std::move(hwmonPath))
{
}

std::vector<SystemTemperature> SystemTemperatureReader::readTemperatures() const
{
    std::vector<SystemTemperature> temperatures;
    std::error_code error;

    for (const auto& device :
         std::filesystem::directory_iterator{m_hwmonPath, error})
    {
        const auto deviceName = readText(device.path() / "name");
        if (!deviceName)
        {
            continue;
        }

        for (const auto& entry :
             std::filesystem::directory_iterator{device.path(), error})
        {
            const std::string filename = entry.path().filename().string();
            if (!filename.starts_with("temp") ||
                !filename.ends_with("_input"))
            {
                continue;
            }

            const auto rawTemperature = readText(entry.path());
            if (!rawTemperature)
            {
                continue;
            }

            try
            {
                std::size_t parsedCharacters = 0;
                const double millidegrees =
                    std::stod(*rawTemperature, &parsedCharacters);
                if (parsedCharacters != rawTemperature->size())
                {
                    continue;
                }

                const double celsius = millidegrees / 1000.0;
                temperatures.push_back({
                    displayName(*deviceName, sensorLabel(entry.path())),
                    celsius * 9.0 / 5.0 + 32.0});
            }
            catch (const std::exception&)
            {
            }
        }
    }

    std::sort(
        temperatures.begin(),
        temperatures.end(),
        [](const SystemTemperature& left, const SystemTemperature& right)
        {
            return left.name < right.name;
        });
    return temperatures;
}