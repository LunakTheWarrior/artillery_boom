#include "ModulinoLight.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <array>

namespace
{
    // ========================================================
    // LTR-381RGB registers
    // ========================================================

    constexpr uint8_t REG_MAIN_CTRL = 0x00;
    constexpr uint8_t REG_MAIN_STATUS = 0x07;

    // IR
    constexpr uint8_t REG_IR_DATA = 0x0A;

    // Green
    constexpr uint8_t REG_GREEN_DATA = 0x0D;

    // Blue
    constexpr uint8_t REG_BLUE_DATA = 0x10;

    // Red
    constexpr uint8_t REG_RED_DATA = 0x13;

    // MAIN_CTRL
    constexpr uint8_t ALS_ENABLE = 0x02;

    // MAIN_STATUS
    constexpr uint8_t ALS_DATA_READY = 0x08;
}


// ============================================================
// Constructor
// ============================================================

ModulinoLight::ModulinoLight()
    : _i2c(nullptr),
      _initialized(false),
      _red(0),
      _green(0),
      _blue(0),
      _ir(0),
      _ambient(0)
{
}


// ============================================================
// Begin
// ============================================================

bool ModulinoLight::begin(
    i2c_inst_t* i2c,
    uint8_t sda_pin,
    uint8_t scl_pin,
    uint32_t baudrate
)
{
    _i2c = i2c;

    // Initialize I2C
    i2c_init(_i2c, baudrate);

    // Configure pins
    gpio_set_function(
        sda_pin,
        GPIO_FUNC_I2C
    );

    gpio_set_function(
        scl_pin,
        GPIO_FUNC_I2C
    );

    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    sleep_ms(10);

    // Check sensor
    if (!connected())
    {
        return false;
    }

    // Enable ALS / colour sensor
    if (!writeRegister(
            REG_MAIN_CTRL,
            ALS_ENABLE))
    {
        return false;
    }

    sleep_ms(100);

    _initialized = true;

    return true;
}


// ============================================================
// Check connection
// ============================================================

bool ModulinoLight::connected()
{
    if (_i2c == nullptr)
    {
        return false;
    }

    uint8_t reg = REG_MAIN_CTRL;

    int result = i2c_write_blocking(
        _i2c,
        I2C_ADDRESS,
        &reg,
        1,
        true
    );

    return result >= 0;
}


// ============================================================
// Write register
// ============================================================

bool ModulinoLight::writeRegister(
    uint8_t reg,
    uint8_t value
)
{
    if (_i2c == nullptr)
    {
        return false;
    }

    uint8_t buffer[2];

    buffer[0] = reg;
    buffer[1] = value;

    int result = i2c_write_blocking(
        _i2c,
        I2C_ADDRESS,
        buffer,
        2,
        false
    );

    return result == 2;
}


// ============================================================
// Read registers
// ============================================================

bool ModulinoLight::readRegisters(
    uint8_t reg,
    uint8_t* buffer,
    size_t length
)
{
    if (_i2c == nullptr)
    {
        return false;
    }

    // Select register
    int result = i2c_write_blocking(
        _i2c,
        I2C_ADDRESS,
        &reg,
        1,
        true
    );

    if (result != 1)
    {
        return false;
    }

    // Read data
    result = i2c_read_blocking(
        _i2c,
        I2C_ADDRESS,
        buffer,
        length,
        false
    );

    return result == static_cast<int>(length);
}


// ============================================================
// Read one register
// ============================================================

bool ModulinoLight::readRegister(
    uint8_t reg,
    uint8_t& value
)
{
    return readRegisters(
        reg,
        &value,
        1
    );
}


// ============================================================
// Wait for measurement
// ============================================================

bool ModulinoLight::waitForData(
    uint32_t timeout_ms
)
{
    absolute_time_t timeout =
        make_timeout_time_ms(timeout_ms);

    while (!time_reached(timeout))
    {
        uint8_t status = 0;

        if (readRegister(
                REG_MAIN_STATUS,
                status))
        {
            if (status & ALS_DATA_READY)
            {
                return true;
            }
        }

        sleep_ms(1);
    }

    return false;
}


// ============================================================
// Update all sensor values
// ============================================================
//
// Reads:
//   IR
//   Green
//   Blue
//   Red
//   Ambient light
//
// The LTR-381RGB has contiguous 3-byte registers for each
// channel, so we read each channel as a 20-bit value.
//

bool ModulinoLight::update()
{
    if (!_initialized)
    {
        return false;
    }

    if (!waitForData())
    {
        return false;
    }

    uint8_t data[3];

    // --------------------------------------------------------
    // Ambient light
    // --------------------------------------------------------

    if (!readRegisters(
            0x0D,
            data,
            3))
    {
        return false;
    }

    _ambient =
        ((uint32_t)data[2] << 16) |
        ((uint32_t)data[1] << 8) |
        data[0];

    _ambient &= 0xFFFFF;


    // --------------------------------------------------------
    // Green
    // --------------------------------------------------------

    if (!readRegisters(
            REG_GREEN_DATA,
            data,
            3))
    {
        return false;
    }

    _green =
        ((uint32_t)data[2] << 16) |
        ((uint32_t)data[1] << 8) |
        data[0];

    _green &= 0xFFFFF;


    // --------------------------------------------------------
    // Blue
    // --------------------------------------------------------

    if (!readRegisters(
            REG_BLUE_DATA,
            data,
            3))
    {
        return false;
    }

    _blue =
        ((uint32_t)data[2] << 16) |
        ((uint32_t)data[1] << 8) |
        data[0];

    _blue &= 0xFFFFF;


    // --------------------------------------------------------
    // Red
    // --------------------------------------------------------

    if (!readRegisters(
            REG_RED_DATA,
            data,
            3))
    {
        return false;
    }

    _red =
        ((uint32_t)data[2] << 16) |
        ((uint32_t)data[1] << 8) |
        data[0];

    _red &= 0xFFFFF;


    // --------------------------------------------------------
    // IR
    // --------------------------------------------------------

    if (!readRegisters(
            REG_IR_DATA,
            data,
            3))
    {
        return false;
    }

    _ir =
        ((uint32_t)data[2] << 16) |
        ((uint32_t)data[1] << 8) |
        data[0];

    _ir &= 0xFFFFF;


    return true;
}


// ============================================================
// Raw ambient light
// ============================================================

uint32_t ModulinoLight::getRawLight()
{
    if (!update())
    {
        return 0;
    }

    return _ambient;
}


// ============================================================
// Lux
// ============================================================

float ModulinoLight::getLux()
{
    if (!update())
    {
        return 0.0f;
    }

    // Simple approximation.
    //
    // For calibrated lux values, gain and integration time
    // should also be taken into account.

    return static_cast<float>(_ambient) / 3.0f;
}


// ============================================================
// Get RGB color
// ============================================================

ModulinoLight::Color ModulinoLight::getColor()
{
    update();

    Color color;

    color.r = _red;
    color.g = _green;
    color.b = _blue;

    return color;
}


// ============================================================
// Individual RGB values
// ============================================================

uint32_t ModulinoLight::getRed()
{
    update();
    return _red;
}


uint32_t ModulinoLight::getGreen()
{
    update();
    return _green;
}


uint32_t ModulinoLight::getBlue()
{
    update();
    return _blue;
}


// ============================================================
// Infrared
// ============================================================

uint32_t ModulinoLight::getIR()
{
    update();
    return _ir;
}

std::uint32_t
ModulinoLight::getIRDirect()
{
    std::array<std::uint8_t, 3> data{};
    if (!readRegisters(
        REG_IR_DATA,
        data.data(),
        data.size()))
    {
        return false;
    }

    std::uint32_t result =
        ((uint32_t)data[2] << 16) |
        ((uint32_t)data[1] << 8) |
        data[0];

    result &= 0xFFFFF;
    return result;
}