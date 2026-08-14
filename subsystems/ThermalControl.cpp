#include "ThermalControl.hpp"

ThermalControl::ThermalControl()
    : PayloadSubsystem(
          "THERMAL_CONTROL",
          "Thermal Control",
          std::chrono::seconds(10),
          20.0,
          0.0,
          0.0004),
      current_temperature_(0.0),
      target_temperature_(0.0),
      temperature_difference_(0.0)
{
}

void ThermalControl::update()
{
    PayloadSubsystem::update();

    // Thermal-control-specific processing
    //
    // Update temperatures, etc.
}

double ThermalControl::currentTemperature() const
{
    return current_temperature_;
}

double ThermalControl::targetTemperature() const
{
    return target_temperature_;
}

double ThermalControl::temperatureDifference() const
{
    return temperature_difference_;
}