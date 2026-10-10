// AVR port of src/ModulinoDistance.cpp; sensor algorithms retained, Wire transport.
// Sensor configuration, timing equations and status mapping derived from ST's
// VL53L4CD ULD. Copyright (c) 2021 STMicroelectronics.
// SPDX-License-Identifier: BSD-3-Clause
// See licenses/STMicroelectronics-BSD-3-Clause.txt and docs/modulino-distance.md.
#include "ModulinoDistance.h"

#include <Arduino.h>
using book::Failure;
namespace reg = vl53l4cd::registers;

namespace {
// ST's mandatory configuration for registers 0x002D..0x0087, inclusive.
// Reserved bytes must keep their manufacturer-provided values.
constexpr book::Array<uint8_t, 91> defaultConfiguration{
    0x12, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
    0x0b, 0x00, 0x00, 0x02, 0x14, 0x21, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00,
    0xc8, 0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00, 0x00, 0x01, 0xcc, 0x07,
    0x01, 0xf1, 0x05, 0x00, 0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00, 0x00,
    0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x05, 0x06,
    0x06, 0x00, 0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
};
static_assert(defaultConfiguration.size() == reg::systemStart.address - reg::i2cFastModePlus.address + 1, "configuration size");

constexpr book::Array<uint8_t, 24> rangeStatuses{
    255, 255, 255, 5, 2, 4, 1, 7, 3, 0, 255, 255,
    9, 13, 255, 255, 255, 255, 10, 6, 255, 255, 11, 12,
};

bool validTiming(ModulinoDistance::Milliseconds budget, ModulinoDistance::Milliseconds interval) {
    return budget >= 10 && budget <= 200 && (interval == 0 || interval > budget);
}



// Each timeout is encoded as an 8-bit mantissa plus an 8-bit power-of-two exponent.
uint16_t encodeTimeout(uint64_t clocks) {
    uint16_t exponent = 0;
    auto mantissa = clocks - 1;
    while (mantissa > 255) {
        mantissa >>= 1;
        ++exponent;
    }
    return static_cast<uint16_t>((exponent << 8) | mantissa);
}
} // namespace

ModulinoDistance::ModulinoDistance(TwoWire& wire) : wire_(wire) {}

ModulinoDistance::Status ModulinoDistance::readBytes(uint16_t address, book::Span<uint8_t> bytes) {
    if (!busConfigured_) return Failure(Error::NotInitialized);
    if (bytes.size() > 32) return Failure(Error::InvalidArgument);
    wire_.clearWireTimeoutFlag();
    wire_.beginTransmission(defaultAddress);
    wire_.write(uint8_t(address >> 8));
    wire_.write(uint8_t(address));
    const uint8_t status = wire_.endTransmission(false);
    if (wire_.getWireTimeoutFlag()) return Failure(Error::I2cTimeout);
    if (status) return Failure(Error::I2cFailure);
    const uint8_t received = wire_.requestFrom(defaultAddress, uint8_t(bytes.size()), uint8_t(true));
    if (wire_.getWireTimeoutFlag()) return Failure(Error::I2cTimeout);
    if (received != bytes.size()) {
        while (wire_.available()) wire_.read();
        return Failure(Error::I2cFailure);
    }
    for (size_t i = 0; i < bytes.size(); ++i) bytes[i] = uint8_t(wire_.read());
    return {};
}

ModulinoDistance::Status ModulinoDistance::writeBytes(uint16_t address, book::Span<const uint8_t> bytes) {
    if (!busConfigured_) return Failure(Error::NotInitialized);
    if (bytes.size() > 4) return Failure(Error::InvalidArgument);
    wire_.clearWireTimeoutFlag();
    wire_.beginTransmission(defaultAddress);
    wire_.write(uint8_t(address >> 8));
    wire_.write(uint8_t(address));
    for (size_t i = 0; i < bytes.size(); ++i) wire_.write(bytes[i]);
    const uint8_t status = wire_.endTransmission(true);
    if (wire_.getWireTimeoutFlag()) return Failure(Error::I2cTimeout);
    if (status) return Failure(Error::I2cFailure);
    return {};
}

ModulinoDistance::Result<uint16_t> ModulinoDistance::readModelId() {
    return readRegister(reg::modelId);
}

ModulinoDistance::Status ModulinoDistance::begin(Milliseconds budget, Milliseconds interval) {
    if (!validTiming(budget, interval)) {
        return book::Failure(Error::InvalidArgument);
    }
    state_ = State::Uninitialized;
    wire_.begin();
    wire_.setClock(busSpeedHz);
    wire_.setWireTimeout(transferTimeoutUs, true);
    busConfigured_ = true;

    const uint32_t started = millis();
    while (true) {
        auto firmware = readRegister(reg::firmwareStatus);
        if (!firmware) {
            return book::Failure(firmware.error());
        }
        if (*firmware == 0x03) {
            break;
        }
        if (uint32_t(millis() - started) >= 1000) {
            return book::Failure(Error::Timeout);
        }
        delay(1);
    }
    auto id = readModelId();
    if (!id) {
        return book::Failure(id.error());
    }
    if (*id != expectedModelId) {
        return book::Failure(Error::WrongDevice);
    }
    { auto status = configure(); if (!status) {
        return failAndStop(status.error());
    } }
    state_ = State::Idle;
    { auto status = setRangeTiming(budget, interval); if (!status) {
        return failAndStop(status.error());
    } }
    return {};
}

ModulinoDistance::Status ModulinoDistance::configure() {
    // Permit explicit reinitialization of an already-running device.
    { auto status = writeRegister(reg::systemStart, 0); if (!status) {
        return status;
    } }
    for (size_t i = 0; i < defaultConfiguration.size(); ++i) {
        const vl53l4cd::Register<uint8_t> address{
            static_cast<uint16_t>(reg::i2cFastModePlus.address + i)};
        { auto status = writeRegister(address, defaultConfiguration[i]); if (!status) {
            return status;
        } }
    }
    // The first ranging cycle performs VHV calibration, not an application sample.
    { auto status = writeRegister(reg::systemStart, 0x40); if (!status) {
        return status;
    } }
    { auto status = waitForData(1000); if (!status) {
        return status;
    } }
    { auto status = writeRegister(reg::interruptClear, 1); if (!status) {
        return status;
    } }
    { auto status = writeRegister(reg::systemStart, 0); if (!status) {
        return status;
    } }
    { auto status = writeRegister(reg::vhvTimeout, 0x09); if (!status) {
        return status;
    } }
    { auto status = writeRegister(reg::vhvInit, 0); if (!status) {
        return status;
    } }
    return writeRegister(reg::dssTargetRate, 0x0500);
}

ModulinoDistance::Status ModulinoDistance::setRangeTiming(Milliseconds budget, Milliseconds interval) {
    if (state_ == State::Uninitialized) {
        return book::Failure(Error::NotInitialized);
    }
    if (state_ == State::Ranging) {
        return book::Failure(Error::AlreadyRanging);
    }
    if (!validTiming(budget, interval)) {
        return book::Failure(Error::InvalidArgument);
    }
    auto oscillator = readRegister(reg::oscillatorFrequency);
    if (!oscillator) {
        return book::Failure(oscillator.error());
    }
    if (*oscillator == 0) {
        return book::Failure(Error::InvalidArgument);
    }

    // ST's fixed-point timing formula; wide intermediates avoid overflow.
    const uint64_t macroPeriod = (2304ULL * (0x40000000ULL / *oscillator)) >> 6;
    auto timingUs = static_cast<uint64_t>(budget) * 1000;
    uint32_t period = 0;
    if (interval == 0) {
        timingUs -= 2500;
    } else {
        auto calibration = readRegister(reg::oscillatorCalibration);
        if (!calibration) {
            return book::Failure(calibration.error());
        }
        const auto clock = *calibration & 0x03FF;
        const auto encoded = static_cast<uint64_t>(interval) * clock * 1055 / 1000;
        if (clock == 0 || encoded > UINT32_MAX) {
            return book::Failure(Error::InvalidArgument);
        }
        period = static_cast<uint32_t>(encoded);
        timingUs = (timingUs - 4300) / 2;
    }
    const auto clocksFor = [=](uint32_t multiplier) {
        const auto divisor = (macroPeriod * multiplier) >> 6;
        return ((timingUs << 12) + divisor / 2) / divisor;
    };
    const auto clocksA = clocksFor(16);
    const auto clocksB = clocksFor(12);
    if (clocksA == 0 || clocksB == 0) {
        return book::Failure(Error::InvalidArgument);
    }
    { auto status = writeRegister(reg::intermeasurementPeriod, period); if (!status) {
        return failAndStop(status.error());
    } }
    { auto status = writeRegister(reg::rangeConfigA, encodeTimeout(clocksA)); if (!status) {
        return failAndStop(status.error());
    } }
    { auto status = writeRegister(reg::rangeConfigB, encodeTimeout(clocksB)); if (!status) {
        return failAndStop(status.error());
    } }
    return {};
}

ModulinoDistance::Result<bool> ModulinoDistance::checkDataReady() {
    auto control = readRegister(reg::gpioMuxControl);
    if (!control) {
        return book::Failure(control.error());
    }
    auto status = readRegister(reg::gpioStatus);
    if (!status) {
        return book::Failure(status.error());
    }
    const bool activeHigh = (*control & 0x10) == 0;
    return ((*status & 0x01) != 0) == activeHigh;
}

ModulinoDistance::Status ModulinoDistance::waitForData(Milliseconds timeout) {
    if (timeout > 0x7FFFFFFFUL) {
        return book::Failure(Error::InvalidArgument);
    }
    const uint32_t started = millis();
    while (true) {
        auto ready = checkDataReady();
        if (!ready) {
            return book::Failure(ready.error());
        }
        if (*ready) {
            return {};
        }
        if (uint32_t(millis() - started) >= timeout) {
            return book::Failure(Error::Timeout);
        }
        delay(1);
    }
}

ModulinoDistance::Status ModulinoDistance::startRanging(Milliseconds timeout) {
    if (state_ == State::Uninitialized) {
        return book::Failure(Error::NotInitialized);
    }
    if (state_ == State::Ranging) {
        return book::Failure(Error::AlreadyRanging);
    }
    if (timeout > 0x7FFFFFFFUL) {
        return book::Failure(Error::InvalidArgument);
    }
    auto period = readRegister(reg::intermeasurementPeriod);
    if (!period) {
        return book::Failure(period.error());
    }
    { auto status = writeRegister(reg::systemStart, *period == 0 ? 0x21 : 0x40); if (!status) {
        return failAndStop(status.error());
    } }
    { auto status = waitForData(timeout); if (!status) {
        return failAndStop(status.error());
    } }
    // ST discards the first result after starting; the next result is fresh.
    { auto status = writeRegister(reg::interruptClear, 1); if (!status) {
        return failAndStop(status.error());
    } }
    state_ = State::Ranging;
    return {};
}

ModulinoDistance::Status ModulinoDistance::stopRanging() {
    if (state_ == State::Uninitialized) {
        return book::Failure(Error::NotInitialized);
    }
    { auto status = writeRegister(reg::systemStart, 0); if (!status) {
        state_ = State::Uninitialized;
        return status;
    } }
    state_ = State::Idle;
    return {};
}

ModulinoDistance::Status ModulinoDistance::failAndStop(Error error) {
    state_ = State::Uninitialized;
    (void)writeRegister(reg::systemStart, 0);
    return book::Failure(error);
}

ModulinoDistance::Result<bool> ModulinoDistance::dataReady() {
    if (state_ == State::Uninitialized) {
        return book::Failure(Error::NotInitialized);
    }
    if (state_ != State::Ranging) {
        return book::Failure(Error::NotRanging);
    }
    return checkDataReady();
}

ModulinoDistance::Result<ModulinoDistance::Measurement> ModulinoDistance::readMeasurement() {
    auto ready = dataReady();
    if (!ready) {
        return book::Failure(ready.error());
    }
    if (!*ready) {
        return book::Failure(Error::NotReady);
    }
    // One burst reads the latched result block before acknowledging the interrupt.
    book::Array<uint8_t, reg::distance.address + 2 - reg::rangeStatus.address> bytes{};
    { auto status = readBytes(reg::rangeStatus.address, bytes); if (!status) {
        return book::Failure(status.error());
    } }
    { auto status = writeRegister(reg::interruptClear, 1); if (!status) {
        return book::Failure(status.error());
    } }
    const auto decodeWord = [&](vl53l4cd::Register<uint16_t> address) {
        const auto offset = address.address - reg::rangeStatus.address;
        return static_cast<uint16_t>((uint16_t(bytes[offset]) << 8) | bytes[offset + 1]);
    };
    const auto rawStatus = static_cast<uint8_t>(bytes.front() & 0x1F);
    Measurement measurement;
    measurement.distanceMm = decodeWord(reg::distance);
    measurement.rangeStatus = rawStatus < rangeStatuses.size() ? rangeStatuses[rawStatus] : uint8_t{255};
    measurement.rawRangeStatus = rawStatus;
    measurement.streamCount = bytes[reg::streamCount.address - reg::rangeStatus.address];
    measurement.numberOfSpads = static_cast<uint16_t>(decodeWord(reg::effectiveSpads) / 256);
    measurement.sigmaMm = decodeWord(reg::sigma) / 4.0f;
    measurement.signalKcps = static_cast<uint32_t>(decodeWord(reg::signalRate)) * 8;
    measurement.ambientKcps = static_cast<uint32_t>(decodeWord(reg::ambientRate)) * 8;
    if (measurement.numberOfSpads == 0) {
        measurement.rangeStatus = 255;
    } else {
        measurement.signalPerSpadKcps = measurement.signalKcps / measurement.numberOfSpads;
        measurement.ambientPerSpadKcps = measurement.ambientKcps / measurement.numberOfSpads;
    }
    return measurement;
}

ModulinoDistance::Result<uint16_t> ModulinoDistance::readDistance(Milliseconds timeout) {
    if (state_ == State::Uninitialized) {
        return book::Failure(Error::NotInitialized);
    }
    if (state_ != State::Ranging) {
        return book::Failure(Error::NotRanging);
    }
    { auto status = waitForData(timeout); if (!status) {
        return book::Failure(status.error());
    } }
    auto measurement = readMeasurement();
    if (!measurement) {
        return book::Failure(measurement.error());
    }
    if (!measurement->valid()) {
        return book::Failure(Error::InvalidMeasurement);
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
