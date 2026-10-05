#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "pitches.h"
#include "ModulinoLight.h"
#include "searcher.h"
#include <thread>

constexpr uint8_t I2C_SDA = 2;
constexpr uint8_t I2C_SCL = 3;

ModulinoLight light;

void lol()
{
    printf("From a thread\n");
    sleep_ms(1000);
}

int main()
{
    stdio_init_all();

    sleep_ms(2000);

    PersonSearcher searcher;

    if (!light.begin(
            i2c1,
            I2C_SDA,
            I2C_SCL,
            100000))
    {
        printf("ERROR: Modulino Light not found!\n");

        while (true)
        {
            sleep_ms(1000);
        }
    }

    printf("Modulino Light connected!\n");

    while (true)
    {
        searcher.findPerson(light.getIRDirect());
        
    }
}
