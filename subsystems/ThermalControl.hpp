#pragma once

#include "../core/Subsystem.hpp"

class ThermalControl : public PayloadSubsystem
{
public:
    ThermalControl();

    void update() override;

    double currentTemperature() const;
    double targetTemperature() const;
    double temperatureDifference() const;

private:
    double current_temperature_;
    double target_temperature_;
    double temperature_difference_;
};