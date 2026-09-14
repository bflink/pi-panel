#pragma once

#include <string>

class Ads1015
{
public:
    explicit Ads1015(std::string devicePath = "/dev/i2c-1",
                     unsigned int address = 0x48);
    ~Ads1015();

    Ads1015(const Ads1015&) = delete;
    Ads1015& operator=(const Ads1015&) = delete;

    double readVoltage(unsigned int channel);

private:
    void ensureOpen();
    void writeRegister(unsigned char registerAddress, unsigned short value);
    unsigned short readRegister(unsigned char registerAddress);

    std::string m_devicePath;
    unsigned int m_address;
    int m_fileDescriptor = -1;
};