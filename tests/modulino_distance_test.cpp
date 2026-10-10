#include "src/ModulinoDistance.h"
#include "pico/stdlib.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std::chrono_literals;
using Sensor = ModulinoDistance;
using Error = Sensor::Error;
namespace reg = vl53l4cd::registers;

namespace {
enum class Operation { Select, Read, Write };
struct Transaction {
    Operation operation;
    std::uint16_t address;
    std::vector<std::uint8_t> bytes;
};
struct Fault {
    Operation operation;
    std::uint16_t address;
    int result;
};
struct FakeSensor {
    std::array<std::uint8_t, 512> memory{};
    std::vector<Transaction> transactions;
    std::optional<Fault> fault;
    std::uint64_t nowUs{};
    std::uint64_t readyAt{};
    std::uint16_t selected{};
    bool awaitingRead{};
    bool running{};
    bool neverReady{};
    bool booted{true};
    bool busInitialized{};
    std::array<bool, 4> pullups{};
    std::array<bool, 4> i2cPins{};

    void word(std::uint16_t address, std::uint16_t value) {
        memory[address] = value >> 8;
        memory[address + 1] = value;
    }
    std::uint16_t word(std::uint16_t address) const {
        return (memory[address] << 8) | memory[address + 1];
    }
    void sample(std::uint16_t distance = 321, std::uint8_t status = 9) {
        memory[0x0089] = status;
        memory[0x008B] = 7;
        word(0x008C, 0x0400); // 4 effective SPADs, 8.8 fixed point
        word(0x008E, 1000);   // 8000 kcps
        word(0x0090, 25);     // 200 kcps
        word(0x0092, 9);      // 2.25 mm sigma
        word(0x0096, distance);
        readyAt = nowUs;
    }
    void reset() {
        *this = FakeSensor{};
        word(0x010F, 0xEBAA);
        word(0x0006, 40000);
        word(0x00DE, 200);
        word(0x001E, 12); // Factory offset must survive initialization.
        sample();
    }
    std::optional<int> fail(Operation operation, std::uint16_t address) {
        if (fault && fault->operation == operation && fault->address == address) {
            const auto result = fault->result;
            fault.reset();
            return result;
        }
        return std::nullopt;
    }
} fake;

Sensor startSensor() {
    fake.reset();
    Sensor sensor(i2c1, 2, 3);
    EXPECT_TRUE(sensor.begin());
    EXPECT_TRUE(sensor.startRanging());
    return sensor;
}

template <typename T>
void checkError(const Sensor::Result<T>& result, Error error) {
    EXPECT_TRUE(!result);
    EXPECT_TRUE(result.error() == error);
}

void initializationAndProtocol() {
    fake.reset();
    Sensor sensor(i2c1, 2, 3);
    checkError(sensor.readModelId(), Error::NotInitialized);
    EXPECT_TRUE(sensor.begin());
    EXPECT_TRUE(fake.busInitialized && fake.pullups[2] && fake.pullups[3]);
    EXPECT_TRUE(fake.i2cPins[2] && fake.i2cPins[3]);
    EXPECT_TRUE(sensor.readModelId() == 0xEBAA);
    EXPECT_TRUE(!fake.running);
    EXPECT_TRUE(fake.word(0x001E) == 12);
    EXPECT_TRUE(fake.memory[0x0008] == 9);
    EXPECT_TRUE(fake.memory[0x000B] == 0);
    EXPECT_TRUE(fake.word(0x0024) == 0x0500);
    // Known ULD vector for oscillator=40000 and timing budget=50 ms.
    EXPECT_TRUE(fake.word(0x005E) == 0x02C9);
    EXPECT_TRUE(fake.word(0x0061) == 0x0386);
    EXPECT_TRUE(fake.word(0x006C) == 0 && fake.word(0x006E) == 0);
    checkError(sensor.readMeasurement(), Error::NotRanging);

    std::vector<Transaction> writes;
    for (const auto& transaction : fake.transactions) {
        if (transaction.operation == Operation::Write) writes.push_back(transaction);
    }
    EXPECT_TRUE(writes.front().address == 0x0087 && writes.front().bytes[0] == 0);
    for (unsigned i = 0; i < 91; ++i) {
        EXPECT_TRUE(writes[i + 1].address == 0x002D + i);
        EXPECT_TRUE(writes[i + 1].bytes.size() == 1);
    }
    EXPECT_TRUE(writes[1].bytes[0] == 0x12);
    EXPECT_TRUE(writes[4].bytes[0] == 0x11);
    EXPECT_TRUE(writes[26].bytes[0] == 0x20);
    EXPECT_TRUE(writes[92].address == 0x0087 && writes[92].bytes[0] == 0x40);
    EXPECT_TRUE(writes[93].address == 0x0086 && writes[93].bytes[0] == 1);
    EXPECT_TRUE(writes[94].address == 0x0087 && writes[94].bytes[0] == 0);
    EXPECT_TRUE(!fake.awaitingRead);
    EXPECT_TRUE(sensor.writeRegister(reg::intermeasurementPeriod, 0x12345678));
    EXPECT_TRUE(fake.word(0x006C) == 0x1234 && fake.word(0x006E) == 0x5678);
    EXPECT_TRUE(sensor.readRegister(reg::intermeasurementPeriod) == 0x12345678);
}

void validAndInvalidMeasurements() {
    auto sensor = startSensor();
    EXPECT_TRUE(fake.memory[0x0087] == 0x21);
    checkError(sensor.readMeasurement(), Error::NotReady);
    fake.sample();
    auto measurement = sensor.readMeasurement();
    EXPECT_TRUE(measurement && measurement->valid());
    EXPECT_TRUE(measurement->distanceMm == 321);
    EXPECT_TRUE(measurement->numberOfSpads == 4);
    EXPECT_TRUE(measurement->signalKcps == 8000 && measurement->signalPerSpadKcps == 2000);
    EXPECT_TRUE(measurement->ambientKcps == 200 && measurement->ambientPerSpadKcps == 50);
    EXPECT_TRUE(measurement->sigmaMm == 2.25f && measurement->streamCount == 7);
    const auto count = fake.transactions.size();
    EXPECT_TRUE(fake.transactions[count - 2].operation == Operation::Read);
    EXPECT_TRUE(fake.transactions[count - 2].address == 0x0089);
    EXPECT_TRUE(fake.transactions[count - 2].bytes.size() == 15);
    EXPECT_TRUE(fake.transactions.back().operation == Operation::Write);
    EXPECT_TRUE(fake.transactions.back().address == 0x0086);
    checkError(sensor.readMeasurement(), Error::NotReady); // No duplicate sample.

    fake.sample(321, 0xE9); // Upper status bits are flags, not part of the status code.
    EXPECT_TRUE(sensor.readMeasurement()->valid());
    fake.sample(0);
    EXPECT_TRUE(sensor.readDistance(0ms) == 0); // A valid zero is distinct from an I/O error.
    fake.sample(500, 6); // Raw status 6 maps to ULD sigma failure 1.
    measurement = sensor.readMeasurement();
    EXPECT_TRUE(measurement && !measurement->valid() && measurement->rangeStatus == 1);
    fake.sample(500, 6);
    checkError(sensor.readDistance(0ms), Error::InvalidMeasurement);
    fake.sample(500, 31); // Unmapped status.
    EXPECT_TRUE(sensor.readMeasurement()->rangeStatus == 255);
    fake.sample(500, 9);
    fake.word(0x008C, 0);
    measurement = sensor.readMeasurement();
    EXPECT_TRUE(measurement && !measurement->valid());
    EXPECT_TRUE(measurement->signalPerSpadKcps == 0);
    fake.sample(500);
    fake.word(0x008E, 65535);
    EXPECT_TRUE(sensor.readMeasurement()->signalKcps == 524280); // No 16-bit overflow.
}

void readinessPolarityAndTimeouts() {
    auto sensor = startSensor();
    fake.neverReady = true;
    EXPECT_TRUE(sensor.dataReady() == false);
    const auto before = fake.nowUs;
    checkError(sensor.readDistance(5ms), Error::Timeout);
    EXPECT_TRUE(fake.nowUs - before < 10000);
    fake.neverReady = false;
    // Verify both GPIO interrupt polarities, independently of result status.
    for (const auto polarity : {0x01, 0x11}) {
        EXPECT_TRUE(sensor.writeRegister(reg::gpioMuxControl, polarity));
        fake.sample();
        EXPECT_TRUE(sensor.dataReady() == true);
        EXPECT_TRUE(sensor.readMeasurement());
        EXPECT_TRUE(sensor.dataReady() == false);
    }
    EXPECT_TRUE(sensor.stopRanging());
    fake.neverReady = true;
    checkError(sensor.startRanging(5ms), Error::Timeout);
    EXPECT_TRUE(!fake.running);
    checkError(sensor.startRanging(), Error::NotInitialized);
    fake.neverReady = false;
    EXPECT_TRUE(sensor.begin());
    EXPECT_TRUE(sensor.startRanging());
}

void timingAndStateValidation() {
    fake.reset();
    Sensor sensor(i2c1, 2, 3);
    checkError(sensor.begin(9ms), Error::InvalidArgument);
    checkError(sensor.begin(201ms), Error::InvalidArgument);
    checkError(sensor.begin(50ms, 50ms), Error::InvalidArgument);
    checkError(sensor.begin(50ms, -1ms), Error::InvalidArgument);
    EXPECT_TRUE(fake.transactions.empty());
    EXPECT_TRUE(sensor.begin(50ms, 100ms));
    EXPECT_TRUE(fake.word(0x006C) == 0 && fake.word(0x006E) == 21100);
    EXPECT_TRUE(fake.word(0x005E) == 0x01C1);
    EXPECT_TRUE(fake.word(0x0061) == 0x0281);
    EXPECT_TRUE(sensor.startRanging());
    EXPECT_TRUE(fake.memory[0x0087] == 0x40);
    checkError(sensor.startRanging(), Error::AlreadyRanging);
    checkError(sensor.setRangeTiming(20ms, 0ms), Error::AlreadyRanging);
    EXPECT_TRUE(sensor.stopRanging());
    checkError(sensor.startRanging(-1ms), Error::InvalidArgument);
    EXPECT_TRUE(sensor.setRangeTiming(20ms, 0ms));
    EXPECT_TRUE(sensor.startRanging());
    EXPECT_TRUE(fake.memory[0x0087] == 0x21);
    EXPECT_TRUE(sensor.stopRanging());
    fake.word(0x0006, 0);
    checkError(sensor.setRangeTiming(20ms, 0ms), Error::InvalidArgument);
    fake.word(0x0006, 40000);
    fake.word(0x00DE, 0);
    checkError(sensor.setRangeTiming(20ms, 100ms), Error::InvalidArgument);
    fake.word(0x00DE, 1023);
    checkError(sensor.setRangeTiming(20ms, Sensor::Milliseconds{0xFFFFFFFF}), Error::InvalidArgument);
}

void initializationFailures() {
    fake.reset();
    Sensor missing(nullptr, 2, 3);
    checkError(missing.begin(), Error::InvalidArgument);
    Sensor sensor(i2c1, 2, 3);
    fake.booted = false;
    checkError(sensor.begin(), Error::Timeout);
    EXPECT_TRUE(fake.nowUs < 1'100'000);
    EXPECT_TRUE(std::ranges::none_of(fake.transactions, [](const auto& t) { return t.operation == Operation::Write; }));
    fake.reset();
    fake.word(0x010F, 0xFFFF);
    checkError(sensor.begin(), Error::WrongDevice);
    EXPECT_TRUE(std::ranges::none_of(fake.transactions, [](const auto& t) { return t.operation == Operation::Write; }));
    fake.reset();
    fake.neverReady = true;
    checkError(sensor.begin(), Error::Timeout);
    EXPECT_TRUE(!fake.running);
    checkError(sensor.startRanging(), Error::NotInitialized);
    fake.reset();
    fake.word(0x0006, 0);
    checkError(sensor.begin(), Error::InvalidArgument);
    checkError(sensor.startRanging(), Error::NotInitialized);
    fake.reset();
    EXPECT_TRUE(sensor.begin()); // Recovery after a failed initialization.
}

void transportFailures() {
    auto sensor = startSensor();
    fake.fault = Fault{Operation::Select, 0x010F, PICO_ERROR_GENERIC};
    const auto before = fake.transactions.size();
    checkError(sensor.readModelId(), Error::I2cFailure);
    EXPECT_TRUE(fake.transactions.size() == before + 1); // Failed selection must not read stale data.
    fake.fault = Fault{Operation::Read, 0x010F, 1}; // Short read.
    checkError(sensor.readModelId(), Error::I2cFailure);
    fake.fault = Fault{Operation::Read, 0x010F, PICO_ERROR_TIMEOUT};
    checkError(sensor.readModelId(), Error::I2cTimeout);
    fake.sample();
    fake.fault = Fault{Operation::Read, 0x0089, 14};
    checkError(sensor.readMeasurement(), Error::I2cFailure);
    EXPECT_TRUE(fake.transactions.back().operation == Operation::Read); // Don't clear an unread sample.
    fake.fault = Fault{Operation::Write, 0x0086, PICO_ERROR_GENERIC};
    checkError(sensor.readMeasurement(), Error::I2cFailure); // Don't publish if acknowledgment failed.
    EXPECT_TRUE(sensor.stopRanging());
    fake.fault = Fault{Operation::Write, 0x005E, 3}; // Partial timing update.
    checkError(sensor.setRangeTiming(20ms, 0ms), Error::I2cFailure);
    checkError(sensor.startRanging(), Error::NotInitialized);
    EXPECT_TRUE(!fake.running);

    // Fail each configuration write in turn: no failed init may allow ranging.
    for (unsigned address = 0x002D; address <= 0x0087; ++address) {
        fake.reset();
        fake.fault = Fault{Operation::Write, static_cast<std::uint16_t>(address), PICO_ERROR_GENERIC};
        checkError(sensor.begin(), Error::I2cFailure);
        checkError(sensor.startRanging(), Error::NotInitialized);
        EXPECT_TRUE(!fake.running);
    }
}
} // namespace

unsigned i2c_init(i2c_inst_t* bus, unsigned baudrate) {
    EXPECT_TRUE(bus == i2c1 && baudrate == 100000);
    fake.busInitialized = true;
    return baudrate;
}
void gpio_set_function(unsigned pin, int function) {
    EXPECT_TRUE((pin == 2 || pin == 3) && function == 3);
    fake.i2cPins[pin] = true;
}
void gpio_pull_up(unsigned pin) { EXPECT_TRUE(pin < 4); fake.pullups[pin] = true; }
absolute_time_t make_timeout_time_ms(std::uint32_t ms) { return fake.nowUs + std::uint64_t{ms} * 1000; }
bool time_reached(absolute_time_t deadline) { return fake.nowUs >= deadline; }
void sleep_ms(std::uint32_t ms) { fake.nowUs += std::uint64_t{ms} * 1000; }

int i2c_write_timeout_us(i2c_inst_t* bus, std::uint8_t address, const std::uint8_t* bytes,
                         std::size_t count, bool noStop, unsigned timeout) {
    EXPECT_TRUE(fake.busInitialized && bus == i2c1 && address == 0x29 && timeout == 10000);
    EXPECT_TRUE(count >= 2 && count <= 6);
    const auto target = static_cast<std::uint16_t>((bytes[0] << 8) | bytes[1]);
    const auto operation = count == 2 ? Operation::Select : Operation::Write;
    EXPECT_TRUE(noStop == (operation == Operation::Select));
    EXPECT_TRUE(target + count - 2 <= fake.memory.size());
    fake.transactions.push_back({operation, target, {bytes + 2, bytes + count}});
    fake.nowUs += 100;
    if (auto failure = fake.fail(operation, target)) {
        fake.awaitingRead = false;
        return *failure;
    }
    if (operation == Operation::Select) {
        EXPECT_TRUE(!fake.awaitingRead);
        fake.selected = target;
        fake.awaitingRead = true;
    } else {
        std::copy(bytes + 2, bytes + count, fake.memory.begin() + target);
        if (target == 0x0087) {
            fake.running = bytes[2] != 0;
            fake.readyAt = fake.nowUs + 5000;
        }
        if (target == 0x0086 && bytes[2] == 1) fake.readyAt = fake.nowUs + 10000;
    }
    return static_cast<int>(count);
}

int i2c_read_timeout_us(i2c_inst_t* bus, std::uint8_t address, std::uint8_t* bytes,
                        std::size_t count, bool noStop, unsigned timeout) {
    EXPECT_TRUE(bus == i2c1 && address == 0x29 && !noStop && timeout == 10000);
    EXPECT_TRUE(fake.awaitingRead && fake.selected + count <= fake.memory.size());
    fake.awaitingRead = false;
    fake.nowUs += 100;
    fake.memory[0x00E5] = fake.booted ? 3 : 0;
    const bool ready = fake.running && !fake.neverReady && fake.nowUs >= fake.readyAt;
    const bool activeHigh = (fake.memory[0x0030] & 0x10) == 0;
    fake.memory[0x0031] = (ready == activeHigh) ? 1 : 0;
    std::copy_n(fake.memory.begin() + fake.selected, count, bytes);
    fake.transactions.push_back({Operation::Read, fake.selected, {bytes, bytes + count}});
    if (auto failure = fake.fail(Operation::Read, fake.selected)) return *failure;
    return static_cast<int>(count);
}

TEST(ModulinoDistanceTest, InitializationAndI2cProtocol) { initializationAndProtocol(); }
TEST(ModulinoDistanceTest, ValidAndInvalidMeasurements) { validAndInvalidMeasurements(); }
TEST(ModulinoDistanceTest, ReadinessPolarityAndTimeouts) { readinessPolarityAndTimeouts(); }
TEST(ModulinoDistanceTest, TimingAndStateValidation) { timingAndStateValidation(); }
TEST(ModulinoDistanceTest, InitializationFailures) { initializationFailures(); }
TEST(ModulinoDistanceTest, TransportFailures) { transportFailures(); }
