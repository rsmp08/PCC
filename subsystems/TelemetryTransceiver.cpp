#include "TelemetryTransceiver.hpp"

TelemetryTransceiver::TelemetryTransceiver()
    : PayloadSubsystem(
          "TELEMETRY_TRANSCEIVER",
          "Telemetry Transceiver",
          std::chrono::seconds(15),
          12.0,
          0.0,
          0.0007),
      signal_strength_(0.0),
      signal_quality_(0.0)
{
}

void TelemetryTransceiver::update()
{
    PayloadSubsystem::update();

    // Telemetry-transceiver-specific processing
    //
    // Update signal strength, quality, noise, etc.
}

double TelemetryTransceiver::signalStrength() const
{
    return signal_strength_;
}

double TelemetryTransceiver::signalQuality() const
{
    return signal_quality_;
}