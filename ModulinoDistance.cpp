#include "ModulinoDistance.h"
#include <cstdio>

std::uint8_t
ModulinoDistance::readModelId()
{
    selectRegister(modelIdRegister_);

    std::uint8_t modelId;

    if (const auto bytesRead = i2c_read_blocking(i2c_,
         i2cAddress_,
          &modelId,
           1,
            true);
    bytesRead != 1)
    {
        printf("Failed to read modelID\n");
        return 0;
    }


    return modelId;
}
