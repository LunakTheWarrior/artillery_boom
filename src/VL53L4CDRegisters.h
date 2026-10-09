#pragma once

#include <concepts>
#include <cstdint>

namespace vl53l4cd {

// Register addresses and multi-byte values are transmitted most-significant byte first.
template <typename T>
concept RegisterValue = std::same_as<T, std::uint8_t> ||
                        std::same_as<T, std::uint16_t> ||
                        std::same_as<T, std::uint32_t>;

template <RegisterValue T>
struct Register {
    std::uint16_t address;
};

namespace registers {
inline constexpr Register<std::uint8_t> softReset{0x0000};
inline constexpr Register<std::uint8_t> i2cAddress{0x0001};
inline constexpr Register<std::uint16_t> oscillatorFrequency{0x0006};
inline constexpr Register<std::uint8_t> vhvTimeout{0x0008};
inline constexpr Register<std::uint8_t> vhvInit{0x000B};
inline constexpr Register<std::uint16_t> xtalkPlaneOffset{0x0016};
inline constexpr Register<std::uint16_t> xtalkXGradient{0x0018};
inline constexpr Register<std::uint16_t> xtalkYGradient{0x001A};
inline constexpr Register<std::uint16_t> rangeOffset{0x001E};
inline constexpr Register<std::uint16_t> innerOffset{0x0020};
inline constexpr Register<std::uint16_t> outerOffset{0x0022};
inline constexpr Register<std::uint16_t> dssTargetRate{0x0024};
inline constexpr Register<std::uint8_t> i2cFastModePlus{0x002D};
inline constexpr Register<std::uint8_t> gpioMuxControl{0x0030};
inline constexpr Register<std::uint8_t> gpioStatus{0x0031};
inline constexpr Register<std::uint8_t> interruptConfig{0x0046};
inline constexpr Register<std::uint16_t> rangeConfigA{0x005E};
inline constexpr Register<std::uint16_t> rangeConfigB{0x0061};
inline constexpr Register<std::uint16_t> sigmaThreshold{0x0064};
inline constexpr Register<std::uint16_t> signalThreshold{0x0066};
inline constexpr Register<std::uint32_t> intermeasurementPeriod{0x006C};
inline constexpr Register<std::uint16_t> distanceThresholdHigh{0x0072};
inline constexpr Register<std::uint16_t> distanceThresholdLow{0x0074};
inline constexpr Register<std::uint8_t> interruptClear{0x0086};
inline constexpr Register<std::uint8_t> systemStart{0x0087};
inline constexpr Register<std::uint8_t> rangeStatus{0x0089};
inline constexpr Register<std::uint8_t> streamCount{0x008B};
inline constexpr Register<std::uint16_t> effectiveSpads{0x008C};
inline constexpr Register<std::uint16_t> signalRate{0x008E};
inline constexpr Register<std::uint16_t> ambientRate{0x0090};
inline constexpr Register<std::uint16_t> sigma{0x0092};
inline constexpr Register<std::uint16_t> distance{0x0096};
inline constexpr Register<std::uint16_t> oscillatorCalibration{0x00DE};
inline constexpr Register<std::uint8_t> firmwareStatus{0x00E5};
inline constexpr Register<std::uint16_t> modelId{0x010F};
} // namespace registers
} // namespace vl53l4cd
