// Sensor configuration, timing equations and status mapping derived from ST's
// VL53L4CD ULD. Copyright (c) 2021 STMicroelectronics.
// SPDX-License-Identifier: BSD-3-Clause
// See licenses/STMicroelectronics-BSD-3-Clause.txt and docs/modulino-distance.md.
#include "ModulinoDistance.h"

#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include <algorithm>
#include <limits>

using namespace std::chrono_literals;
namespace reg = vl53l4cd::registers;

namespace {
// ST's mandatory configuration for registers 0x002D..0x0087, inclusive.
// Reserved bytes must keep their manufacturer-provided values.
constexpr std::array<std::uint8_t, 91> defaultConfiguration{
    0x12, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
    0x0b, 0x00, 0x00, 0x02, 0x14, 0x21, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00,
    0xc8, 0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00, 0x00, 0x01, 0xcc, 0x07,
    0x01, 0xf1, 0x05, 0x00, 0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00, 0x00,
    0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x05, 0x06,
    0x06, 0x00, 0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
};
static_assert(defaultConfiguration.size() == reg::systemStart.address - reg::i2cFastModePlus.address + 1);

constexpr std::array<std::uint8_t, 24> rangeStatuses{
    255, 255, 255, 5, 2, 4, 1, 7, 3, 0, 255, 255,
    9, 13, 255, 255, 255, 255, 10, 6, 255, 255, 11, 12,
};

bool validTiming(ModulinoDistance::Milliseconds budget, ModulinoDistance::Milliseconds interval) {
    return budget >= 10ms && budget <= 200ms && interval >= 0ms &&
           (interval == 0ms || interval > budget) &&
           interval.count() <= std::numeric_limits<std::uint32_t>::max();
}

ModulinoDistance::Error transferError(int result) {
    return result == PICO_ERROR_TIMEOUT ? ModulinoDistance::Error::I2cTimeout
                                        : ModulinoDistance::Error::I2cFailure;
}

// Each timeout is encoded as an 8-bit mantissa plus an 8-bit power-of-two exponent.
std::uint16_t encodeTimeout(std::uint64_t clocks) {
    std::uint16_t exponent = 0;
    auto mantissa = clocks - 1;
    while (mantissa > 255) {
        mantissa >>= 1;
        ++exponent;
    }
    return static_cast<std::uint16_t>((exponent << 8) | mantissa);
}
} // namespace

ModulinoDistance::ModulinoDistance(i2c_inst_t* i2c, std::uint32_t sdaPin, std::uint32_t sclPin)
    : i2c_(i2c), sdaPin_(sdaPin), sclPin_(sclPin) {}

ModulinoDistance::Status ModulinoDistance::readBytes(std::uint16_t address, std::span<std::uint8_t> bytes) {
    if (!busConfigured_) {
        return std::unexpected(Error::NotInitialized);
    }
    const std::array index{static_cast<std::uint8_t>(address >> 8), static_cast<std::uint8_t>(address)};
    const auto written = i2c_write_timeout_us(i2c_, defaultAddress, index.data(), index.size(), true, transferTimeoutUs);
    if (written != static_cast<int>(index.size())) {
        return std::unexpected(transferError(written));
    }
    // Repeated START between index and data; STOP at the end of the read.
    const auto read = i2c_read_timeout_us(i2c_, defaultAddress, bytes.data(), bytes.size(), false, transferTimeoutUs);
    if (read != static_cast<int>(bytes.size())) {
        return std::unexpected(transferError(read));
    }
    return {};
}

ModulinoDistance::Status ModulinoDistance::writeBytes(std::uint16_t address, std::span<const std::uint8_t> bytes) {
    if (!busConfigured_) {
        return std::unexpected(Error::NotInitialized);
    }
    if (bytes.size() > sizeof(std::uint32_t)) {
        return std::unexpected(Error::InvalidArgument);
    }
    std::array<std::uint8_t, 6> packet{};
    packet[0] = static_cast<std::uint8_t>(address >> 8);
    packet[1] = static_cast<std::uint8_t>(address);
    std::ranges::copy(bytes, packet.begin() + 2);
    const auto length = bytes.size() + 2;
    const auto written = i2c_write_timeout_us(i2c_, defaultAddress, packet.data(), length, false, transferTimeoutUs);
    if (written != static_cast<int>(length)) {
        return std::unexpected(transferError(written));
    }
    return {};
}

ModulinoDistance::Result<std::uint16_t> ModulinoDistance::readModelId() {
    return readRegister(reg::modelId);
}

ModulinoDistance::Status ModulinoDistance::begin(Milliseconds budget, Milliseconds interval) {
    if (i2c_ == nullptr || !validTiming(budget, interval)) {
        return std::unexpected(Error::InvalidArgument);
    }
    state_ = State::Uninitialized;
    i2c_init(i2c_, busSpeedHz);
    gpio_set_function(sdaPin_, GPIO_FUNC_I2C);
    gpio_set_function(sclPin_, GPIO_FUNC_I2C);
    gpio_pull_up(sdaPin_);
    gpio_pull_up(sclPin_);
    busConfigured_ = true;

    const auto deadline = make_timeout_time_ms(1000);
    while (true) {
        auto firmware = readRegister(reg::firmwareStatus);
        if (!firmware) {
            return std::unexpected(firmware.error());
        }
        if (*firmware == 0x03) {
            break;
        }
        if (time_reached(deadline)) {
            return std::unexpected(Error::Timeout);
        }
        sleep_ms(1);
    }
    auto id = readModelId();
    if (!id) {
        return std::unexpected(id.error());
    }
    if (*id != expectedModelId) {
        return std::unexpected(Error::WrongDevice);
    }
    if (auto status = configure(); !status) {
        return failAndStop(status.error());
    }
    state_ = State::Idle;
    if (auto status = setRangeTiming(budget, interval); !status) {
        return failAndStop(status.error());
    }
    return {};
}

ModulinoDistance::Status ModulinoDistance::configure() {
    // Permit explicit reinitialization of an already-running device.
    if (auto status = writeRegister(reg::systemStart, 0); !status) {
        return status;
    }
    for (std::size_t i = 0; i < defaultConfiguration.size(); ++i) {
        const vl53l4cd::Register<std::uint8_t> address{
            static_cast<std::uint16_t>(reg::i2cFastModePlus.address + i)};
        if (auto status = writeRegister(address, defaultConfiguration[i]); !status) {
            return status;
        }
    }
    // The first ranging cycle performs VHV calibration, not an application sample.
    if (auto status = writeRegister(reg::systemStart, 0x40); !status) {
        return status;
    }
    if (auto status = waitForData(1000ms); !status) {
        return status;
    }
    if (auto status = writeRegister(reg::interruptClear, 1); !status) {
        return status;
    }
    if (auto status = writeRegister(reg::systemStart, 0); !status) {
        return status;
    }
    if (auto status = writeRegister(reg::vhvTimeout, 0x09); !status) {
        return status;
    }
    if (auto status = writeRegister(reg::vhvInit, 0); !status) {
        return status;
    }
    return writeRegister(reg::dssTargetRate, 0x0500);
}

ModulinoDistance::Status ModulinoDistance::setRangeTiming(Milliseconds budget, Milliseconds interval) {
    if (state_ == State::Uninitialized) {
        return std::unexpected(Error::NotInitialized);
    }
    if (state_ == State::Ranging) {
        return std::unexpected(Error::AlreadyRanging);
    }
    if (!validTiming(budget, interval)) {
        return std::unexpected(Error::InvalidArgument);
    }
    auto oscillator = readRegister(reg::oscillatorFrequency);
    if (!oscillator) {
        return std::unexpected(oscillator.error());
    }
    if (*oscillator == 0) {
        return std::unexpected(Error::InvalidArgument);
    }

    // ST's fixed-point timing formula; wide intermediates avoid overflow.
    const std::uint64_t macroPeriod = (2304ULL * (0x40000000ULL / *oscillator)) >> 6;
    auto timingUs = static_cast<std::uint64_t>(budget.count()) * 1000;
    std::uint32_t period = 0;
    if (interval == 0ms) {
        timingUs -= 2500;
    } else {
        auto calibration = readRegister(reg::oscillatorCalibration);
        if (!calibration) {
            return std::unexpected(calibration.error());
        }
        const auto clock = *calibration & 0x03FF;
        const auto encoded = static_cast<std::uint64_t>(interval.count()) * clock * 1055 / 1000;
        if (clock == 0 || encoded > std::numeric_limits<std::uint32_t>::max()) {
            return std::unexpected(Error::InvalidArgument);
        }
        period = static_cast<std::uint32_t>(encoded);
        timingUs = (timingUs - 4300) / 2;
    }
    const auto clocksFor = [=](std::uint32_t multiplier) {
        const auto divisor = (macroPeriod * multiplier) >> 6;
        return ((timingUs << 12) + divisor / 2) / divisor;
    };
    const auto clocksA = clocksFor(16);
    const auto clocksB = clocksFor(12);
    if (clocksA == 0 || clocksB == 0) {
        return std::unexpected(Error::InvalidArgument);
    }
    if (auto status = writeRegister(reg::intermeasurementPeriod, period); !status) {
        return failAndStop(status.error());
    }
    if (auto status = writeRegister(reg::rangeConfigA, encodeTimeout(clocksA)); !status) {
        return failAndStop(status.error());
    }
    if (auto status = writeRegister(reg::rangeConfigB, encodeTimeout(clocksB)); !status) {
        return failAndStop(status.error());
    }
    return {};
}

ModulinoDistance::Result<bool> ModulinoDistance::checkDataReady() {
    auto control = readRegister(reg::gpioMuxControl);
    if (!control) {
        return std::unexpected(control.error());
    }
    auto status = readRegister(reg::gpioStatus);
    if (!status) {
        return std::unexpected(status.error());
    }
    const bool activeHigh = (*control & 0x10) == 0;
    return ((*status & 0x01) != 0) == activeHigh;
}

ModulinoDistance::Status ModulinoDistance::waitForData(Milliseconds timeout) {
    if (timeout < 0ms || timeout.count() > std::numeric_limits<std::uint32_t>::max()) {
        return std::unexpected(Error::InvalidArgument);
    }
    const auto deadline = make_timeout_time_ms(static_cast<std::uint32_t>(timeout.count()));
    while (true) {
        auto ready = checkDataReady();
        if (!ready) {
            return std::unexpected(ready.error());
        }
        if (*ready) {
            return {};
        }
        if (time_reached(deadline)) {
            return std::unexpected(Error::Timeout);
        }
        sleep_ms(1);
    }
}

ModulinoDistance::Status ModulinoDistance::startRanging(Milliseconds timeout) {
    if (state_ == State::Uninitialized) {
        return std::unexpected(Error::NotInitialized);
    }
    if (state_ == State::Ranging) {
        return std::unexpected(Error::AlreadyRanging);
    }
    if (timeout < 0ms || timeout.count() > std::numeric_limits<std::uint32_t>::max()) {
        return std::unexpected(Error::InvalidArgument);
    }
    auto period = readRegister(reg::intermeasurementPeriod);
    if (!period) {
        return std::unexpected(period.error());
    }
    if (auto status = writeRegister(reg::systemStart, *period == 0 ? 0x21 : 0x40); !status) {
        return failAndStop(status.error());
    }
    if (auto status = waitForData(timeout); !status) {
        return failAndStop(status.error());
    }
    // ST discards the first result after starting; the next result is fresh.
    if (auto status = writeRegister(reg::interruptClear, 1); !status) {
        return failAndStop(status.error());
    }
    state_ = State::Ranging;
    return {};
}

ModulinoDistance::Status ModulinoDistance::stopRanging() {
    if (state_ == State::Uninitialized) {
        return std::unexpected(Error::NotInitialized);
    }
    if (auto status = writeRegister(reg::systemStart, 0); !status) {
        state_ = State::Uninitialized;
        return status;
    }
    state_ = State::Idle;
    return {};
}

ModulinoDistance::Status ModulinoDistance::failAndStop(Error error) {
    state_ = State::Uninitialized;
    (void)writeRegister(reg::systemStart, 0);
    return std::unexpected(error);
}

ModulinoDistance::Result<bool> ModulinoDistance::dataReady() {
    if (state_ == State::Uninitialized) {
        return std::unexpected(Error::NotInitialized);
    }
    if (state_ != State::Ranging) {
        return std::unexpected(Error::NotRanging);
    }
    return checkDataReady();
}

ModulinoDistance::Result<ModulinoDistance::Measurement> ModulinoDistance::readMeasurement() {
    auto ready = dataReady();
    if (!ready) {
        return std::unexpected(ready.error());
    }
    if (!*ready) {
        return std::unexpected(Error::NotReady);
    }
    // One burst reads the latched result block before acknowledging the interrupt.
    std::array<std::uint8_t, reg::distance.address + 2 - reg::rangeStatus.address> bytes{};
    if (auto status = readBytes(reg::rangeStatus.address, bytes); !status) {
        return std::unexpected(status.error());
    }
    if (auto status = writeRegister(reg::interruptClear, 1); !status) {
        return std::unexpected(status.error());
    }
    const auto word = [&](vl53l4cd::Register<std::uint16_t> address) {
        const auto offset = address.address - reg::rangeStatus.address;
        return static_cast<std::uint16_t>((bytes[offset] << 8) | bytes[offset + 1]);
    };
    const auto rawStatus = static_cast<std::uint8_t>(bytes.front() & 0x1F);
    Measurement measurement{
        .distanceMm = word(reg::distance),
        .rangeStatus = rawStatus < rangeStatuses.size() ? rangeStatuses[rawStatus] : std::uint8_t{255},
        .rawRangeStatus = rawStatus,
        .streamCount = bytes[reg::streamCount.address - reg::rangeStatus.address],
        .numberOfSpads = static_cast<std::uint16_t>(word(reg::effectiveSpads) / 256),
        .sigmaMm = word(reg::sigma) / 4.0f,
        .signalKcps = static_cast<std::uint32_t>(word(reg::signalRate)) * 8,
        .ambientKcps = static_cast<std::uint32_t>(word(reg::ambientRate)) * 8,
    };
    if (measurement.numberOfSpads == 0) {
        measurement.rangeStatus = 255;
    } else {
        measurement.signalPerSpadKcps = measurement.signalKcps / measurement.numberOfSpads;
        measurement.ambientPerSpadKcps = measurement.ambientKcps / measurement.numberOfSpads;
    }
    return measurement;
}

ModulinoDistance::Result<std::uint16_t> ModulinoDistance::readDistance(Milliseconds timeout) {
    if (state_ == State::Uninitialized) {
        return std::unexpected(Error::NotInitialized);
    }
    if (state_ != State::Ranging) {
        return std::unexpected(Error::NotRanging);
    }
    if (auto status = waitForData(timeout); !status) {
        return std::unexpected(status.error());
    }
    auto measurement = readMeasurement();
    if (!measurement) {
        return std::unexpected(measurement.error());
    }
    if (!measurement->valid()) {
        return std::unexpected(Error::InvalidMeasurement);
    }
    return measurement->distanceMm;
}

const char* ModulinoDistance::errorName(Error error) {
    switch (error) {
    case Error::I2cFailure: return "I2C transfer failed";
    case Error::I2cTimeout: return "I2C transfer timed out";
    case Error::Timeout: return "sensor readiness timed out";
    case Error::WrongDevice: return "unsupported sensor (expected VL53L4CD ID 0xEBAA)";
    case Error::InvalidArgument: return "invalid timing or oscillator value";
    case Error::NotInitialized: return "sensor needs initialization";
    case Error::NotRanging: return "sensor is stopped";
    case Error::AlreadyRanging: return "stop ranging before reconfiguration";
    case Error::NotReady: return "no new sample";
    case Error::InvalidMeasurement: return "invalid distance measurement";
    }
    return "unknown sensor error";
}
