#include <gtest/gtest.h>

#include "Controllers/SystemTemperatureReader.h"

#include <filesystem>
#include <fstream>

namespace
{
    class SystemTemperatureReaderTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_root = std::filesystem::temp_directory_path() /
                ("pi-panel-hwmon-" + std::to_string(::testing::UnitTest::GetInstance()
                    ->random_seed()));
            std::filesystem::remove_all(m_root);
            std::filesystem::create_directories(m_root);
        }

        void TearDown() override
        {
            std::filesystem::remove_all(m_root);
        }

        void addSensor(const std::string& directory,
                       const std::string& deviceName,
                       const std::string& input,
                       const std::string& value,
                       const std::string& label = "")
        {
            const auto devicePath = m_root / directory;
            std::filesystem::create_directories(devicePath);
            std::ofstream{devicePath / "name"} << deviceName;
            std::ofstream{devicePath / input} << value;
            if (!label.empty())
            {
                std::string labelFile = input;
                labelFile.replace(labelFile.size() - 6, 6, "label");
                std::ofstream{devicePath / labelFile} << label;
            }
        }

        std::filesystem::path m_root;
    };
}

TEST_F(SystemTemperatureReaderTest, DiscoversAndNamesHwmonSensors)
{
    addSensor("hwmon7", "cpu_thermal", "temp1_input", "50000");
    addSensor("hwmon2", "rp1_adc", "temp1_input", "40000");
    addSensor("hwmon9", "nvme", "temp1_input", "45000", "Composite");
    addSensor("hwmon9", "nvme", "temp3_input", "46000", "Sensor 2");

    const auto readings =
        SystemTemperatureReader{m_root.string()}.readTemperatures();

    ASSERT_EQ(readings.size(), 4u);
    EXPECT_EQ(readings[0].name, "CPU");
    EXPECT_NEAR(readings[0].fahrenheit, 122.0, 0.01);
    EXPECT_EQ(readings[1].name, "NVMe");
    EXPECT_EQ(readings[2].name, "NVMe Sensor 2");
    EXPECT_EQ(readings[3].name, "RP1 controller");
}

TEST_F(SystemTemperatureReaderTest, IgnoresMalformedAndNonTemperatureFiles)
{
    addSensor("hwmon0", "cpu_thermal", "temp1_input", "invalid");
    addSensor("hwmon1", "pwmfan", "pwm1", "128");

    const auto readings =
        SystemTemperatureReader{m_root.string()}.readTemperatures();

    EXPECT_TRUE(readings.empty());
}