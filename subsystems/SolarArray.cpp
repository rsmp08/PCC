#include "SolarArray.hpp"

SolarArray::SolarArray()
    : PayloadSubsystem(
          "SOLAR_ARRAY",
          "Solar Array",
          std::chrono::seconds(8),
          5.0,
          850.0,
          0.0005),
      efficiency_(1.0)
{
}

void SolarArray::update()
{
    // Run generic subsystem logic first
    PayloadSubsystem::update();

    // Solar-array-specific behavior
    //
    // Example:
    // efficiency could change according to degradation,
    // sun exposure, eclipse, etc.
}

double SolarArray::generatedPower() const
{
    return powerGeneration() * efficiency_;
}