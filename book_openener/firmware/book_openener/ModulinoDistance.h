// AVR port of src/ModulinoDistance.h; sensor algorithms retained, Wire transport.
#pragma once

#include "VL53L4CDRegisters.h"
#include <Wire.h>
#include "Portable.h"


class ModulinoDistance {
public:
    using Milliseconds = uint32_t;

    using Error = book::Error;

    template <typename T>
    using Result = book::Result<T>;
    using Status = Result<void>;

    struct Measurement {
        uint16_t distanceMm{};
        // ST ULD status codes: only 0 is a valid distance; 255 is unknown/invalid.
        uint8_t rangeStatus{255};
        uint8_t rawRangeStatus{};
        uint8_t streamCount{};
        uint16_t numberOfSpads{};
        float sigmaMm{};
        uint32_t signalKcps{};
        uint32_t ambientKcps{};
        uint32_t signalPerSpadKcps{};
        uint32_t ambientPerSpadKcps{};

        bool valid() const { return rangeStatus == 0; }
    };

    static constexpr uint8_t defaultAddress = 0x29; // Pico uses 7-bit addresses.
    static constexpr uint16_t expectedModelId = 0xEBAA;

    explicit ModulinoDistance(TwoWire& wire = Wire);

    // Boot, identify, configure and calibrate. Leaves the device stopped.
    Status begin(Milliseconds budget = Milliseconds{50},
                               Milliseconds interval = Milliseconds{0});
    // budget: 10..200 ms; interval: 0 (continuous), or strictly greater than budget.
    // Stop ranging before changing timing. The factory calibration is retained.
    Status setRangeTiming(Milliseconds budget, Milliseconds interval);
    Status startRanging(Milliseconds timeout = Milliseconds{1000});
    Status stopRanging();
    Result<bool> dataReady();
    // Nonblocking: NotReady means no fresh sample. Invalid samples still carry diagnostics.
    Result<Measurement> readMeasurement();
    // Bounded blocking convenience API; rejects invalid samples, including zero-SPAD results.
    Result<uint16_t> readDistance(Milliseconds timeout = Milliseconds{1000});
    Result<uint16_t> readModelId();
    static const char* errorName(Error error);

    // Low-level diagnostics after begin() has configured the I2C bus. Writing these
    // directly can invalidate the driver's state; use the higher-level APIs normally.
    template <typename T>
    Result<T> readRegister(vl53l4cd::Register<T> reg) {
        book::Array<uint8_t, sizeof(T)> bytes{};
        { auto status = readBytes(reg.address, bytes); if (!status) {
            return book::Failure(status.error());
        } }
        T value{};
        for (const auto byte : bytes) {
            value = static_cast<T>((value << 8) | byte);
        }
        return value;
    }

    template <typename T>
    Status writeRegister(vl53l4cd::Register<T> reg, typename book::Identity<T>::type value) {
        book::Array<uint8_t, sizeof(T)> bytes{};
        for (size_t i = 0; i < bytes.size(); ++i) {
            bytes[bytes.size() - 1 - i] = static_cast<uint8_t>(value >> (8 * i));
        }
        return writeBytes(reg.address, bytes);
    }

private:
    enum class State { Uninitialized, Idle, Ranging };
    static constexpr uint32_t busSpeedHz = 100000;
    static constexpr uint32_t transferTimeoutUs = 10000;

    Status readBytes(uint16_t address, book::Span<uint8_t> bytes);
    Status writeBytes(uint16_t address, book::Span<const uint8_t> bytes);
    Result<bool> checkDataReady();
    Status waitForData(Milliseconds timeout);
    Status configure();
    // Best-effort stop after an uncertain hardware state; preserve the original error.
    Status failAndStop(Error error);

    TwoWire& wire_;
    State state_{State::Uninitialized};
    bool busConfigured_{false};
};
