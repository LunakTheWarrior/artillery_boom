#pragma once

#include <stdint.h>
#include <stddef.h>
#include <cstdint>
#include "hardware/i2c.h"

class ModulinoLight
{
public:

    static constexpr uint8_t I2C_ADDRESS = 0x53;

    // RGB color
    struct Color
    {
        uint32_t r;
        uint32_t g;
        uint32_t b;
    };

    ModulinoLight();

    bool begin(
        i2c_inst_t* i2c,
        uint8_t sda_pin,
        uint8_t scl_pin,
        uint32_t baudrate = 100000
    );

    bool connected();

    // Ambient light
    uint32_t getRawLight();
    float getLux();

    // RGB
    Color getColor();

    uint32_t getRed();
    uint32_t getGreen();
    uint32_t getBlue();

    // Infrared
    uint32_t getIR();
    std::uint32_t getIRDirect();

private:

    i2c_inst_t* _i2c;
    bool _initialized;

    // Last sensor values
    uint32_t _red;
    uint32_t _green;
    uint32_t _blue;
    uint32_t _ir;
    uint32_t _ambient;

    bool writeRegister(
        uint8_t reg,
        uint8_t value
    );

    bool readRegisters(
        uint8_t reg,
        uint8_t* buffer,
        size_t length
    );

    bool readRegister(
        uint8_t reg,
        uint8_t& value
    );

    bool waitForData(
        uint32_t timeout_ms = 100
    );

    bool update();
};
