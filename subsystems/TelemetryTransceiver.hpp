#pragma once

#include "PayloadSubsystem.hpp"

class TelemetryTransceiver : public PayloadSubsystem
{
public:
    TelemetryTransceiver();

    void update() override;

    double signalStrength() const;
    double signalQuality() const;

private:
    double signal_strength_;
    double signal_quality_;
};
