#include <cstdint>
#include "hardware/i2c.h"
#include <memory>
#include "hardware/gpio.h"
#include <bit>

class ModulinoDistance
{
public:
    ModulinoDistance(i2c_inst_t* i2c, const std::uint32_t sdaPin, const std::uint32_t sclPin)
        : sdaPin_(sdaPin)
        , sclPin_(sclPin)
        , i2c_(i2c)
    {
        i2c_init(i2c_, baudraute_);
        gpio_set_function(sdaPin_, GPIO_FUNC_I2C);
        gpio_set_function(sclPin_, GPIO_FUNC_I2C);
        gpio_pull_up(sdaPin_);
        gpio_pull_up(sclPin_);
        sleep_ms(100);
    }

    std::uint8_t
    readModelId();

    template <class T>
    void
    selectRegister(const T);


private:
    static constexpr std::uint32_t baudraute_{100'000};
    static constexpr std::uint16_t modelIdRegister_       = 0x010F;
    static constexpr std::uint16_t interruptStatusRegister_ = 0x0031;
    static constexpr std::uint16_t interruptClearRegister_  = 0x0086;
    static constexpr std::uint16_t systemStartRegister_     = 0x0087;
    static constexpr std::uint16_t rangeStatusRegister_     = 0x0089;
    static constexpr std::uint16_t distanceRegister_        = 0x0096;
    static constexpr std::uint8_t i2cAddress_ = 0x29;

    std::uint32_t sdaPin_;
    std::uint32_t sclPin_;
    i2c_inst_t* i2c_{nullptr};
};

template <class T>
inline void ModulinoDistance::selectRegister(const T reg)
{
    const T flippedVal = std::byteswap(reg);
    auto* addr = reinterpret_cast<const std::uint8_t*>(&flippedVal);
    constexpr auto size = sizeof(T);

    if (const auto resultCode = i2c_write_blocking(
                                    i2c_,
                                    i2cAddress_,
                                    addr,
                                    size,
                                    true
                                ); resultCode != size)
    {
        printf("Failed to select register: %d\n", reg);
    }
}
