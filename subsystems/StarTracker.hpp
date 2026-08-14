#pragma once

#include "PayloadSubsystem.hpp"

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