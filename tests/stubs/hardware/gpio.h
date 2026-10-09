#pragma once
inline constexpr int GPIO_FUNC_I2C = 3;
inline constexpr bool GPIO_OUT = true;
void gpio_init(unsigned);
void gpio_set_dir(unsigned, bool);
void gpio_put(unsigned, bool);
void gpio_set_function(unsigned, int);
void gpio_pull_up(unsigned);
