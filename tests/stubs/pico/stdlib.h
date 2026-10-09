#pragma once
#include <cstdint>
inline constexpr int PICO_ERROR_GENERIC = -1;
inline constexpr int PICO_ERROR_TIMEOUT = -2;
using absolute_time_t = std::uint64_t;
absolute_time_t make_timeout_time_ms(std::uint32_t);
bool time_reached(absolute_time_t);
void sleep_ms(std::uint32_t);
