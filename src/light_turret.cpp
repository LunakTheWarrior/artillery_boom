#include <cstdio>

#include "pico/stdlib.h"
#include "ModulinoDistance.h"
#include "searcher.h"

using namespace std::chrono_literals;

int main() {
    stdio_init_all();
    sleep_ms(2000);

    ModulinoDistance distance(i2c1, 2, 3);
    PersonSearcher searcher;

    while (true) {
        searcher.reset();
        auto started = distance.begin(20ms).and_then([&] { return distance.startRanging(); });
        if (!started) {
            std::printf("Distance sensor: %s; retrying\n", ModulinoDistance::errorName(started.error()));
            sleep_ms(1000);
            continue;
        }
        std::printf("VL53L4CD initialized; distance readings are in millimeters\n");

        auto sampleDeadline = make_timeout_time_ms(1000);
        bool discardSample = true;
        while (true) {
            auto measurement = distance.readMeasurement();
            if (measurement) {
                sampleDeadline = make_timeout_time_ms(1000);
                // Discard the result that may have integrated during movement.
                // The next conversion is taken with the motor stationary.
                if (discardSample) {
                    discardSample = false;
                    continue;
                }
                if (measurement->valid()) {
                    std::printf("Distance: %u mm (sigma %.2f mm, signal %lu kcps)\n",
                                static_cast<unsigned>(measurement->distanceMm),
                                static_cast<double>(measurement->sigmaMm),
                                static_cast<unsigned long>(measurement->signalKcps));
                } else {
                    std::printf("Invalid distance sample: status=%u, raw=%u\n",
                                static_cast<unsigned>(measurement->rangeStatus),
                                static_cast<unsigned>(measurement->rawRangeStatus));
                }
                searcher.observe(measurement->valid()
                    ? std::optional{measurement->distanceMm} : std::nullopt);
                sleep_ms(25); // Settle and finish any in-flight 20 ms conversion.
                discardSample = true;
            } else if (measurement.error() != ModulinoDistance::Error::NotReady) {
                std::printf("Distance sensor: %s; restarting\n", ModulinoDistance::errorName(measurement.error()));
                (void)distance.stopRanging();
                sleep_ms(1000);
                break;
            }
            if (time_reached(sampleDeadline)) {
                std::printf("Distance sensor stopped producing samples; restarting\n");
                (void)distance.stopRanging();
                sleep_ms(1000);
                break;
            }
            sleep_ms(1);
        }
    }
}
