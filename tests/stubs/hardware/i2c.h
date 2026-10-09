#pragma once
#include <cstddef>
#include <cstdint>
struct i2c_inst_t {};
inline i2c_inst_t testI2c;
inline i2c_inst_t* i2c1 = &testI2c;
unsigned i2c_init(i2c_inst_t*, unsigned baudrate);
int i2c_write_timeout_us(i2c_inst_t*, std::uint8_t, const std::uint8_t*, std::size_t, bool, unsigned);
int i2c_read_timeout_us(i2c_inst_t*, std::uint8_t, std::uint8_t*, std::size_t, bool, unsigned);
