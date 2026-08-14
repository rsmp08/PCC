#pragma once

#include "PayloadSubsystem.hpp"

class ADCSGyros : public PayloadSubsystem
{
    public:
        ADCSGyros();

        void update() override;

        double angularRateX() const;
        double angularRateY() const;
        double angularRateZ() const;

    private:
        double angular_rate_x_;
        double angular_rate_y_;
        double angular_rate_z_;
};
