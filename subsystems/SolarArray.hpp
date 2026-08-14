#pragma once

#include "../core/Subsystem.hpp"

class SolarArray : public PayloadSubsystem
{
public:
    SolarArray();

    void update() override;

    double generatedPower() const;

private:
    double efficiency_;
};