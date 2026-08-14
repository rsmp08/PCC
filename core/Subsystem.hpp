#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <string_view>

enum class SubsystemState
{
    OFFLINE,
    INITIALIZING,
    NOMINAL,
    DEGRADED,
    FAILED,
    REBOOTING,
    PATCHING
};

enum class FaultSeverity
{
    NONE,
    TRANSIENT_GLITCH,
    COMPONENT_DEGRADATION,
    CRITICAL_FAULT
};

class PayloadSubsystem
{
public:
    PayloadSubsystem(
        std::string identifier,
        std::string label,
        std::chrono::seconds initialization_duration,
        double power_draw,
        double power_generation,
        double failure_risk_modifier);

    virtual ~PayloadSubsystem() = default;

    virtual void update();

    void beginInitialization();

    void forceFault(FaultSeverity severity);

    void beginReboot();
    void beginPatch(std::string_view patch_type);

    bool rebootComplete() const;
    bool patchComplete() const;

    void completeReboot(bool success);
    void completePatch();

    SubsystemState state() const;
    FaultSeverity faultSeverity() const;

    const std::string &identifier() const;
    const std::string &label() const;

    double powerDraw() const;
    double powerGeneration() const;

    double health() const;
    double telemetryAccuracy() const;

    bool isActive() const;
    bool isOperational() const;
    bool isCritical() const;

protected:
    // Derived subsystems can use these
    double health_;
    double telemetry_accuracy_;

private:
    std::string identifier_;
    std::string label_;

    std::chrono::seconds initialization_duration_;

    double power_draw_;
    double power_generation_;
    double failure_risk_modifier_;

    mutable std::mutex mutex_;

    SubsystemState state_;
    FaultSeverity fault_severity_;

    double power_draw_multiplier_;

    std::chrono::steady_clock::time_point transition_start_;

    std::chrono::seconds reboot_duration_;
    std::chrono::seconds patch_duration_;

    std::string active_patch_;
};