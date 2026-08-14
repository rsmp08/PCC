#pragma once

#include "../core/Subsystem.hpp"

class StarTracker : public PayloadSubsystem
{
public:
    explicit StarTracker(
        std::string identifier,
        std::string label);

    void update() override;

    double attitudeAccuracy() const;

private:
    double attitude_accuracy_;
};