#include "PowerManager.hpp"

#include <stdexcept>

PowerManager::PowerManager()
    : generation_w_(0.0),
      consumption_w_(0.0),
      battery_capacity_wh_(0.0),
      battery_charge_wh_(0.0)
{
}

void PowerManager::update(double delta_time_seconds)
{
    if (delta_time_seconds <= 0.0)
    {
        throw std::invalid_argument("Delta time must be greater than zero");
    }

    updateBattery(delta_time_seconds);
}

double PowerManager::getPowerMargin() const
{
    return generation_w_ - consumption_w_;
}

void PowerManager::setGeneration(double watts)
{
    if (watts < 0.0)
    {
        throw std::invalid_argument("Generation cannot be negative");
    }
    generation_w_ = watts;
}

void PowerManager::setConsumption(double watts)
{
    if (watts < 0.0)
    {
        throw std::invalid_argument("Consumption cannot be negative");
    }
    consumption_w_ = watts;
}

void PowerManager::setBatteryCapacity(double watt_hours)
{
    if (watt_hours < 0.0)
    {
        throw std::invalid_argument("Battery capacity cannot be negative");
    }

    if (battery_charge_wh_ > watt_hours)
    {
        throw std::invalid_argument("Battery charge cannot exceed new capacity");
    }

    battery_capacity_wh_ = watt_hours;
}

void PowerManager::setBatteryCharge(double watt_hours)
{
    if (watt_hours < 0.0)
    {
        throw std::invalid_argument("Battery charge cannot be negative");
    }
    if (watt_hours > battery_capacity_wh_)
    {
        throw std::invalid_argument("Battery charge cannot exceed capacity");
    }
    battery_charge_wh_ = watt_hours;
}

double PowerManager::getBatteryCharge() const
{
    return battery_charge_wh_;
}

double PowerManager::getBatteryCapacity() const
{
    return battery_capacity_wh_;
}

double PowerManager::getBatteryChargePercentage() const
{
    if (battery_capacity_wh_ == 0.0)
    {
        return 0.0;
    }
    return (battery_charge_wh_ / battery_capacity_wh_) * 100.0;
}

void PowerManager::updateBattery(double delta_time_seconds)
{
    const double power_margin_w = getPowerMargin();

    const double delta_time_hours = delta_time_seconds / 3600.0;

    const double energy_change_wh =
        power_margin_w * delta_time_hours;

    battery_charge_wh_ += energy_change_wh;

    if (battery_charge_wh_ > battery_capacity_wh_)
    {
        battery_charge_wh_ = battery_capacity_wh_;
    }

    if (battery_charge_wh_ < 0.0)
    {
        battery_charge_wh_ = 0.0;
    }
}