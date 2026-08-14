#include "Subsystem.hpp"

#include <algorithm>
#include <utility>

PayloadSubsystem::PayloadSubsystem(
    std::string identifier,
    std::string label,
    std::chrono::seconds initialization_duration,
    double power_draw,
    double power_generation,
    double failure_risk_modifier)
    : identifier_(std::move(identifier)),
      label_(std::move(label)),
      initialization_duration_(initialization_duration),
      power_draw_(power_draw),
      power_generation_(power_generation),
      failure_risk_modifier_(failure_risk_modifier),
      state_(SubsystemState::OFFLINE),
      fault_severity_(FaultSeverity::NONE),
      health_(100.0),
      power_draw_multiplier_(1.0),
      telemetry_accuracy_(1.0),
      reboot_duration_(5),
      patch_duration_(8)
{
}

void PayloadSubsystem::beginInitialization()
{
    std::lock_guard lock(mutex_);

    if (state_ != SubsystemState::OFFLINE)
    {
        return;
    }

    state_ = SubsystemState::INITIALIZING;
    transition_start_ = std::chrono::steady_clock::now();
}

void PayloadSubsystem::update()
{
    std::lock_guard lock(mutex_);

    const auto now = std::chrono::steady_clock::now();

    if (state_ == SubsystemState::INITIALIZING)
    {

        const auto elapsed =
            std::chrono::duration_cast<std::chrono::seconds>(
                now - transition_start_);

        if (elapsed >= initialization_duration_)
        {
            state_ = SubsystemState::NOMINAL;
        }
    }

    if (state_ == SubsystemState::REBOOTING)
    {
        // Completion is deliberately handled by SubsystemManager.
        return;
    }

    if (state_ == SubsystemState::PATCHING)
    {
        // Completion is deliberately handled by SubsystemManager.
        return;
    }
}

void PayloadSubsystem::forceFault(
    FaultSeverity severity)
{
    std::lock_guard lock(mutex_);

    fault_severity_ = severity;

    switch (severity)
    {

    case FaultSeverity::TRANSIENT_GLITCH:
        health_ = std::max(health_ - 2.0, 0.0);
        state_ = SubsystemState::DEGRADED;
        break;

    case FaultSeverity::COMPONENT_DEGRADATION:
        health_ = std::max(health_ - 20.0, 0.0);
        state_ = SubsystemState::DEGRADED;
        power_draw_multiplier_ = 1.25;
        telemetry_accuracy_ = 0.75;
        break;

    case FaultSeverity::CRITICAL_FAULT:
        health_ = std::max(health_ - 50.0, 0.0);
        state_ = SubsystemState::FAILED;
        power_draw_multiplier_ = 0.0;
        telemetry_accuracy_ = 0.0;
        break;

    case FaultSeverity::NONE:
        break;
    }
}

void PayloadSubsystem::beginReboot()
{
    std::lock_guard lock(mutex_);

    if (state_ != SubsystemState::FAILED &&
        state_ != SubsystemState::DEGRADED)
    {
        return;
    }

    state_ = SubsystemState::REBOOTING;
    transition_start_ = std::chrono::steady_clock::now();
}

void PayloadSubsystem::beginPatch(
    std::string_view patch_type)
{
    std::lock_guard lock(mutex_);

    if (state_ != SubsystemState::FAILED &&
        state_ != SubsystemState::DEGRADED)
    {
        return;
    }

    active_patch_ = std::string(patch_type);

    state_ = SubsystemState::PATCHING;
    transition_start_ = std::chrono::steady_clock::now();
}

bool PayloadSubsystem::rebootComplete() const
{
    std::lock_guard lock(mutex_);

    if (state_ != SubsystemState::REBOOTING)
    {
        return false;
    }

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() -
            transition_start_);

    return elapsed >= reboot_duration_;
}

bool PayloadSubsystem::patchComplete() const
{
    std::lock_guard lock(mutex_);

    if (state_ != SubsystemState::PATCHING)
    {
        return false;
    }

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() -
            transition_start_);

    return elapsed >= patch_duration_;
}

void PayloadSubsystem::completeReboot(
    bool success)
{
    std::lock_guard lock(mutex_);

    if (success)
    {
        state_ = SubsystemState::INITIALIZING;

        fault_severity_ = FaultSeverity::NONE;
        power_draw_multiplier_ = 1.0;
        telemetry_accuracy_ = 1.0;

        transition_start_ =
            std::chrono::steady_clock::now();
    }
    else
    {
        state_ = SubsystemState::FAILED;
    }
}

void PayloadSubsystem::completePatch()
{
    std::lock_guard lock(mutex_);

    health_ = std::max(health_, 90.0);

    state_ = SubsystemState::INITIALIZING;

    fault_severity_ = FaultSeverity::NONE;
    power_draw_multiplier_ = 1.0;
    telemetry_accuracy_ = 1.0;

    active_patch_.clear();

    transition_start_ =
        std::chrono::steady_clock::now();
}

SubsystemState PayloadSubsystem::state() const
{
    std::lock_guard lock(mutex_);
    return state_;
}

FaultSeverity PayloadSubsystem::faultSeverity() const
{
    std::lock_guard lock(mutex_);
    return fault_severity_;
}

std::string PayloadSubsystem::stateString() const
{
    switch (state())
    {

    case SubsystemState::OFFLINE:
        return "OFFLINE";

    case SubsystemState::INITIALIZING:
        return "INIT";

    case SubsystemState::NOMINAL:
        return "NOMINAL";

    case SubsystemState::DEGRADED:
        return "DEGRADED";

    case SubsystemState::FAILED:
        return "FAILED";

    case SubsystemState::REBOOTING:
        return "REBOOTING";

    case SubsystemState::PATCHING:
        return "PATCHING";
    }

    return "UNKNOWN";
}

std::string PayloadSubsystem::severityString() const
{
    switch (faultSeverity())
    {

    case FaultSeverity::NONE:
        return "NONE";

    case FaultSeverity::TRANSIENT_GLITCH:
        return "GLITCH";

    case FaultSeverity::COMPONENT_DEGRADATION:
        return "DEGRADATION";

    case FaultSeverity::CRITICAL_FAULT:
        return "CRITICAL";
    }

    return "UNKNOWN";
}

const std::string &PayloadSubsystem::identifier() const
{
    return identifier_;
}

const std::string &PayloadSubsystem::label() const
{
    return label_;
}

double PayloadSubsystem::initializationProgress() const
{
    std::lock_guard lock(mutex_);

    if (state_ == SubsystemState::NOMINAL)
    {
        return 1.0;
    }

    if (state_ != SubsystemState::INITIALIZING)
    {
        return 0.0;
    }

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() -
            transition_start_);

    return std::clamp(
        static_cast<double>(elapsed.count()) /
            static_cast<double>(
                initialization_duration_.count() * 1000),
        0.0,
        1.0);
}

double PayloadSubsystem::patchProgress() const
{
    std::lock_guard lock(mutex_);

    if (state_ != SubsystemState::PATCHING)
    {
        return 0.0;
    }

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() -
            transition_start_);

    return std::clamp(
        static_cast<double>(elapsed.count()) /
            static_cast<double>(
                patch_duration_.count() * 1000),
        0.0,
        1.0);
}

double PayloadSubsystem::rebootProgress() const
{
    std::lock_guard lock(mutex_);

    if (state_ != SubsystemState::REBOOTING)
    {
        return 0.0;
    }

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() -
            transition_start_);

    return std::clamp(
        static_cast<double>(elapsed.count()) /
            static_cast<double>(
                reboot_duration_.count() * 1000),
        0.0,
        1.0);
}

double PayloadSubsystem::powerDraw() const
{
    std::lock_guard lock(mutex_);

    if (state_ == SubsystemState::FAILED ||
        state_ == SubsystemState::OFFLINE)
    {
        return 0.0;
    }

    return power_draw_ * power_draw_multiplier_;
}

double PayloadSubsystem::powerGeneration() const
{
    std::lock_guard lock(mutex_);

    if (state_ != SubsystemState::NOMINAL &&
        state_ != SubsystemState::DEGRADED)
    {
        return 0.0;
    }

    return power_generation_;
}

double PayloadSubsystem::health() const
{
    std::lock_guard lock(mutex_);
    return health_;
}

double PayloadSubsystem::failureRiskModifier() const
{
    std::lock_guard lock(mutex_);
    return failure_risk_modifier_;
}

void PayloadSubsystem::setHealth(double value)
{
    std::lock_guard lock(mutex_);
    health_ = std::clamp(value, 0.0, 100.0);
}

void PayloadSubsystem::setPowerDrawMultiplier(double multiplier)
{
    std::lock_guard lock(mutex_);
    power_draw_multiplier_ = std::max(0.0, multiplier);
}

void PayloadSubsystem::setTelemetryAccuracy(double accuracy)
{
    std::lock_guard lock(mutex_);
    telemetry_accuracy_ = std::clamp(accuracy, 0.0, 1.0);
}

double PayloadSubsystem::telemetryAccuracy() const
{
    std::lock_guard lock(mutex_);
    return telemetry_accuracy_;
}

bool PayloadSubsystem::isActive() const
{
    const auto current = state();

    return current == SubsystemState::INITIALIZING ||
           current == SubsystemState::NOMINAL ||
           current == SubsystemState::DEGRADED ||
           current == SubsystemState::REBOOTING ||
           current == SubsystemState::PATCHING;
}

bool PayloadSubsystem::isOperational() const
{
    const auto current = state();

    return current == SubsystemState::NOMINAL ||
           current == SubsystemState::DEGRADED;
}

bool PayloadSubsystem::isCritical() const
{
    return identifier_ == "ADCS_GYROS" ||
           identifier_ == "THERMAL_CONTROL" ||
           identifier_ == "POWER_MANAGER";
}

void PayloadSubsystem::update(std::chrono::seconds /*dt*/)
{
    // Backwards-compatible overload; ignore dt and call the single-step update()
    update();
}

void PayloadSubsystem::degrade()
{
    forceFault(FaultSeverity::COMPONENT_DEGRADATION);
}

void PayloadSubsystem::fail()
{
    forceFault(FaultSeverity::CRITICAL_FAULT);
}

void PayloadSubsystem::reboot()
{
    beginReboot();
}
