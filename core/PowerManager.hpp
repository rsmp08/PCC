#pragma once

class PowerManager
{
public:
    PowerManager();

    // Update
    void update(double delta_time_seconds);

    // Power
    void setGeneration(double watts);
    void setConsumption(double watts);
    double getPowerMargin() const;

    // Battery
    void setBatteryCapacity(double watt_hours);
    void setBatteryCharge(double watt_hours);

    double getBatteryCharge() const;
    double getBatteryCapacity() const;
    double getBatteryChargePercentage() const;

private:
    void updateBattery(double delta_time_seconds);

    double generation_w_;
    double consumption_w_;

    double battery_capacity_wh_;
    double battery_charge_wh_;
};