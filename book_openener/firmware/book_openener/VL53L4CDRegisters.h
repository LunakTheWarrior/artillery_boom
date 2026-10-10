#pragma once

#include <stdint.h>

namespace vl53l4cd {

// Register addresses and multi-byte values are transmitted most-significant byte first.

template <typename T>
struct Register {
    uint16_t address;
};

namespace registers {
constexpr Register<uint8_t> softReset{0x0000};
constexpr Register<uint8_t> i2cAddress{0x0001};
constexpr Register<uint16_t> oscillatorFrequency{0x0006};
constexpr Register<uint8_t> vhvTimeout{0x0008};
constexpr Register<uint8_t> vhvInit{0x000B};
constexpr Register<uint16_t> xtalkPlaneOffset{0x0016};
constexpr Register<uint16_t> xtalkXGradient{0x0018};
constexpr Register<uint16_t> xtalkYGradient{0x001A};
constexpr Register<uint16_t> rangeOffset{0x001E};
constexpr Register<uint16_t> innerOffset{0x0020};
constexpr Register<uint16_t> outerOffset{0x0022};
constexpr Register<uint16_t> dssTargetRate{0x0024};
constexpr Register<uint8_t> i2cFastModePlus{0x002D};
constexpr Register<uint8_t> gpioMuxControl{0x0030};
constexpr Register<uint8_t> gpioStatus{0x0031};
constexpr Register<uint8_t> interruptConfig{0x0046};
constexpr Register<uint16_t> rangeConfigA{0x005E};
constexpr Register<uint16_t> rangeConfigB{0x0061};
constexpr Register<uint16_t> sigmaThreshold{0x0064};
constexpr Register<uint16_t> signalThreshold{0x0066};
constexpr Register<uint32_t> intermeasurementPeriod{0x006C};
constexpr Register<uint16_t> distanceThresholdHigh{0x0072};
constexpr Register<uint16_t> distanceThresholdLow{0x0074};
constexpr Register<uint8_t> interruptClear{0x0086};
constexpr Register<uint8_t> systemStart{0x0087};
constexpr Register<uint8_t> rangeStatus{0x0089};
constexpr Register<uint8_t> streamCount{0x008B};
constexpr Register<uint16_t> effectiveSpads{0x008C};
constexpr Register<uint16_t> signalRate{0x008E};
constexpr Register<uint16_t> ambientRate{0x0090};
constexpr Register<uint16_t> sigma{0x0092};
constexpr Register<uint16_t> distance{0x0096};
constexpr Register<uint16_t> oscillatorCalibration{0x00DE};
constexpr Register<uint8_t> firmwareStatus{0x00E5};
constexpr Register<uint16_t> modelId{0x010F};
} // namespace registers
} // namespace vl53l4cd
