#include "Ads1015.h"

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <thread>

namespace
{
    constexpr unsigned char CONVERSION_REGISTER = 0x00;
    constexpr unsigned char CONFIG_REGISTER = 0x01;
    constexpr unsigned short CONVERSION_READY = 0x8000;
    constexpr unsigned short SINGLE_SHOT_MODE = 0x0100;
    constexpr unsigned short SAMPLE_RATE_250 = 0x0020;
    constexpr unsigned short DISABLE_COMPARATOR = 0x0003;
    constexpr double FULL_SCALE_VOLTS = 6.144;

    constexpr unsigned short channelConfig(unsigned int channel)
    {
        return static_cast<unsigned short>((0x04U + channel) << 12U);
    }

    std::runtime_error systemError(const std::string& operation)
    {
        return std::runtime_error(operation + ": " + std::strerror(errno));
    }
}

Ads1015::Ads1015(std::string devicePath, unsigned int address)
    : m_devicePath(std::move(devicePath)),
      m_address(address)
{
}

Ads1015::~Ads1015()
{
    if (m_fileDescriptor >= 0)
    {
        close(m_fileDescriptor);
    }
}

double Ads1015::readVoltage(unsigned int channel)
{
    if (channel > 3)
    {
        throw std::out_of_range("ADS1015 channel must be between 0 and 3");
    }

    ensureOpen();

    const unsigned short config = static_cast<unsigned short>(
        CONVERSION_READY |
        channelConfig(channel) |
        SINGLE_SHOT_MODE |
        SAMPLE_RATE_250 |
        DISABLE_COMPARATOR);

    writeRegister(CONFIG_REGISTER, config);
    const auto conversionStarted = std::chrono::steady_clock::now();

    while ((readRegister(CONFIG_REGISTER) & CONVERSION_READY) == 0)
    {
        if (std::chrono::steady_clock::now() - conversionStarted >
            std::chrono::milliseconds{100})
        {
            throw std::runtime_error("Timed out waiting for Explorer HAT ADC");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }

    const auto conversionTime = std::chrono::steady_clock::now() - conversionStarted;
    const unsigned short conversion = readRegister(CONVERSION_REGISTER);

    double voltage;
    if (conversionTime < std::chrono::milliseconds{60})
    {
        int value = conversion >> 4U;
        if ((value & 0x800) != 0)
        {
            value -= 1 << 12;
        }
        voltage = static_cast<double>(value) * FULL_SCALE_VOLTS / 2047.0;
    }
    else
    {
        int value = conversion;
        if ((value & 0x8000) != 0)
        {
            value -= 1 << 16;
        }
        voltage = static_cast<double>(value) * FULL_SCALE_VOLTS / 32767.0;
    }

    return std::max(0.0, voltage);
}

void Ads1015::ensureOpen()
{
    if (m_fileDescriptor >= 0)
    {
        return;
    }

    m_fileDescriptor = open(m_devicePath.c_str(), O_RDWR);
    if (m_fileDescriptor < 0)
    {
        throw systemError("Cannot open " + m_devicePath);
    }

    if (ioctl(m_fileDescriptor, I2C_SLAVE, m_address) < 0)
    {
        const auto error = systemError("Cannot select Explorer HAT ADC at 0x48");
        close(m_fileDescriptor);
        m_fileDescriptor = -1;
        throw error;
    }
}

void Ads1015::writeRegister(unsigned char registerAddress, unsigned short value)
{
    const unsigned char data[] = {
        registerAddress,
        static_cast<unsigned char>(value >> 8U),
        static_cast<unsigned char>(value & 0xffU)};

    if (write(m_fileDescriptor, data, sizeof(data)) != sizeof(data))
    {
        throw systemError("Cannot write Explorer HAT ADC register");
    }
}

unsigned short Ads1015::readRegister(unsigned char registerAddress)
{
    if (write(m_fileDescriptor, &registerAddress, 1) != 1)
    {
        throw systemError("Cannot select Explorer HAT ADC register");
    }

    unsigned char data[2]{};
    if (read(m_fileDescriptor, data, sizeof(data)) != sizeof(data))
    {
        throw systemError("Cannot read Explorer HAT ADC register");
    }

    return static_cast<unsigned short>(
        (static_cast<unsigned short>(data[0]) << 8U) | data[1]);
}