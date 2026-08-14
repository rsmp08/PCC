#include "ADCSGyros.hpp"

ADCSGyros::ADCSGyros()
    : PayloadSubsystem(
          "ADCS_GYROS",
          "ADCS Gyroscopes",
          std::chrono::seconds(12),
          35.0,
          0.0,
          0.0008),
      angular_rate_x_(0.0),
      angular_rate_y_(0.0),
      angular_rate_z_(0.0)
{
}

void ADCSGyros::update()
{
    PayloadSubsystem::update();

    // Gyroscope-specific processing
    //
    // Update angular velocity measurements,
    // sensor noise, drift, calibration, etc.
}

double ADCSGyros::angularRateX() const
{
    return angular_rate_x_;
}

double ADCSGyros::angularRateY() const
{
    return angular_rate_y_;
}

double ADCSGyros::angularRateZ() const
{
    return angular_rate_z_;
}