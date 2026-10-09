#pragma once

#include "VL53L4CDRegisters.h"
#include "hardware/i2c.h"

#include <array>
#include <chrono>
#include <expected>
#include <span>
#include <type_traits>

class ModulinoDistance {
public:
    using Milliseconds = std::chrono::milliseconds;

    enum class Error {
        I2cFailure,
        I2cTimeout,
        Timeout,
        WrongDevice,
        InvalidArgument,
        NotInitialized,
        NotRanging,
        AlreadyRanging,
        NotReady,
        InvalidMeasurement,
    };

    template <typename T>
    using Result = std::expected<T, Error>;
    using Status = Result<void>;

    struct Measurement {
        std::uint16_t distanceMm{};
        // ST ULD status codes: only 0 is a valid distance; 255 is unknown/invalid.
        std::uint8_t rangeStatus{255};
        std::uint8_t rawRangeStatus{};
        std::uint8_t streamCount{};
        std::uint16_t numberOfSpads{};
        float sigmaMm{};
        std::uint32_t signalKcps{};
        std::uint32_t ambientKcps{};
        std::uint32_t signalPerSpadKcps{};
        std::uint32_t ambientPerSpadKcps{};

        [[nodiscard]] bool valid() const { return rangeStatus == 0; }
    };

    static constexpr std::uint8_t defaultAddress = 0x29; // Pico uses 7-bit addresses.
    static constexpr std::uint16_t expectedModelId = 0xEBAA;

    ModulinoDistance(i2c_inst_t* i2c, std::uint32_t sdaPin, std::uint32_t sclPin);

    // Boot, identify, configure and calibrate. Leaves the device stopped.
    [[nodiscard]] Status begin(Milliseconds budget = Milliseconds{50},
                               Milliseconds interval = Milliseconds{0});
    // budget: 10..200 ms; interval: 0 (continuous), or strictly greater than budget.
    // Stop ranging before changing timing. The factory calibration is retained.
    [[nodiscard]] Status setRangeTiming(Milliseconds budget, Milliseconds interval);
    [[nodiscard]] Status startRanging(Milliseconds timeout = Milliseconds{1000});
    [[nodiscard]] Status stopRanging();
    [[nodiscard]] Result<bool> dataReady();
    // Nonblocking: NotReady means no fresh sample. Invalid samples still carry diagnostics.
    [[nodiscard]] Result<Measurement> readMeasurement();
    // Bounded blocking convenience API; rejects invalid samples, including zero-SPAD results.
    [[nodiscard]] Result<std::uint16_t> readDistance(Milliseconds timeout = Milliseconds{1000});
    [[nodiscard]] Result<std::uint16_t> readModelId();
    [[nodiscard]] static const char* errorName(Error error);

    // Low-level diagnostics after begin() has configured the I2C bus. Writing these
    // directly can invalidate the driver's state; use the higher-level APIs normally.
    template <vl53l4cd::RegisterValue T>
    [[nodiscard]] Result<T> readRegister(vl53l4cd::Register<T> reg) {
        std::array<std::uint8_t, sizeof(T)> bytes{};
        if (auto status = readBytes(reg.address, bytes); !status) {
            return std::unexpected(status.error());
        }
        T value{};
        for (const auto byte : bytes) {
            value = static_cast<T>((value << 8) | byte);
        }
        return value;
    }

    template <vl53l4cd::RegisterValue T>
    [[nodiscard]] Status writeRegister(vl53l4cd::Register<T> reg, std::type_identity_t<T> value) {
        std::array<std::uint8_t, sizeof(T)> bytes{};
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            bytes[bytes.size() - 1 - i] = static_cast<std::uint8_t>(value >> (8 * i));
        }
        return writeBytes(reg.address, bytes);
    }

private:
    enum class State { Uninitialized, Idle, Ranging };
    static constexpr std::uint32_t busSpeedHz = 100'000;
    static constexpr std::uint32_t transferTimeoutUs = 10'000;

    [[nodiscard]] Status readBytes(std::uint16_t address, std::span<std::uint8_t> bytes);
    [[nodiscard]] Status writeBytes(std::uint16_t address, std::span<const std::uint8_t> bytes);
    [[nodiscard]] Result<bool> checkDataReady();
    [[nodiscard]] Status waitForData(Milliseconds timeout);
    [[nodiscard]] Status configure();
    // Best-effort stop after an uncertain hardware state; preserve the original error.
    [[nodiscard]] Status failAndStop(Error error);

    i2c_inst_t* i2c_;
    std::uint32_t sdaPin_;
    std::uint32_t sclPin_;
    State state_{State::Uninitialized};
    bool busConfigured_{false};
};
