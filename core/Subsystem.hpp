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

    void beginInitialization();
    void update();
    // Compatibility overload used by older callers
    void update(std::chrono::seconds /*dt*/);

    void forceFault(FaultSeverity severity);

    // Convenience helpers used in older UI/controller code
    void degrade();
    void fail();
    void reboot();

    void beginReboot();
    void beginPatch(std::string_view patch_type);

    bool rebootComplete() const;
    bool patchComplete() const;

    void completeReboot(bool success);
    void completePatch();

    SubsystemState state() const;
    FaultSeverity faultSeverity() const;

    std::string stateString() const;
    std::string severityString() const;

    const std::string &identifier() const;
    const std::string &label() const;

    double initializationProgress() const;
    double patchProgress() const;
    double rebootProgress() const;

    double powerDraw() const;
    double powerGeneration() const;

    double health() const;
    double failureRiskModifier() const;

    void setHealth(double value);
    void setPowerDrawMultiplier(double multiplier);
    void setTelemetryAccuracy(double accuracy);

    double telemetryAccuracy() const;

    bool isActive() const;
    bool isOperational() const;
    bool isCritical() const;

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

    double health_;
    double power_draw_multiplier_;
    double telemetry_accuracy_;

    std::chrono::steady_clock::time_point operation_start_;
    std::chrono::steady_clock::time_point transition_start_;

    std::chrono::seconds reboot_duration_;
    std::chrono::seconds patch_duration_;

    std::string active_patch_;
};
