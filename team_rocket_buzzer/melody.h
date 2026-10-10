#pragma once
#include <array>
#include <cstdint>

struct Note {
    std::uint16_t hz;       // Zero is a rest.
    std::uint16_t duration_ms;
};

// User-supplied nine-note motif, repeated without adding a continuation.
// 120 BPM: quarter note = 500 ms; half note = 1000 ms.
inline constexpr std::uint16_t quarter_ms = 500;
inline constexpr std::array<Note, 9> melody = {{
    {440, quarter_ms},     // A4
    {523, quarter_ms},     // C5
    {494, quarter_ms},     // B4
    {392, quarter_ms},     // G4
    {440, 2 * quarter_ms}, // A4
    {440, quarter_ms},     // A4
    {523, quarter_ms},     // C5
    {494, quarter_ms},     // B4
    {330, 2 * quarter_ms}, // E4
}};
