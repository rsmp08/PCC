#pragma once

#include "PayloadSubsystem.hpp"

class SolarArray : public PayloadSubsystem
{
public:
    SolarArray();

    void update() override;

    double generatedPower() const;

private:
    double efficiency_;
};