#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "pitches.h"
#include "ModulinoDistance.h"
#include "searcher.h"
#include <thread>



int main()
{
    stdio_init_all();

    ModulinoDistance distance(i2c1, 2, 3);

    sleep_ms(2000);

    PersonSearcher searcher;

    const auto modelIdForDistanceThing = distance.readModelId();
    printf("The modeasdasdasdlid i read was: %d\n", modelIdForDistanceThing);
    const std::uint16_t modelType = distance.readRegister<std::uint16_t>(std::uint16_t(0x010F));
    printf("Modeltype: %d\n", modelType);

    while (true)
    {
        searcher.findPerson(10);
    }
}
