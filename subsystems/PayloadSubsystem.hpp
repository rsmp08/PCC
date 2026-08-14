#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

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
        double failure_risk_modifier,
        std::vector<int> parameters = {});

    virtual ~PayloadSubsystem() = default;

    // Main subsystem update.
    // Derived subsystems override this and should normally call
    // PayloadSubsystem::update() first.
    virtual void update();

    // Backwards-compatible overload.
    void update(std::chrono::seconds dt);

    // Lifecycle
    void beginInitialization();

    void beginReboot();
    void beginPatch(std::string_view patch_type);

    bool rebootComplete() const;
    bool patchComplete() const;

    void completeReboot(bool success);
    void completePatch();

    // Fault handling
    void forceFault(FaultSeverity severity);

    void degrade();
    void fail();
    void reboot();

    // State
    SubsystemState state() const;
    FaultSeverity faultSeverity() const;

    std::string stateString() const;
    std::string severityString() const;

    // Identity
    const std::string &identifier() const;
    const std::string &label() const;

    // Progress
    double initializationProgress() const;
    double patchProgress() const;
    double rebootProgress() const;

    // Power
    double powerDraw() const;
    double powerGeneration() const;

    // Health / telemetry
    double health() const;
    void setHealth(double value);

    double failureRiskModifier() const;

    void setPowerDrawMultiplier(double multiplier);
    void setTelemetryAccuracy(double accuracy);

    double telemetryAccuracy() const;

    // Status helpers
    bool isActive() const;
    bool isOperational() const;
    bool isCritical() const;

protected:
    /*
     * These values are protected because derived subsystem classes
     * may need to use them for subsystem-specific behavior.
     *
     * Example:
     *
     *   health_ -= 0.1;
     *
     * A derived class should still use the generic functions where
     * possible so state remains consistent.
     */
    double health_;
    double telemetry_accuracy_;

private:
    std::string identifier_;
    std::string label_;

    std::chrono::seconds initialization_duration_;

    double power_draw_;
    double power_generation_;
    double failure_risk_modifier_;

    std::vector<int> parameters_;

    mutable std::mutex mutex_;

    SubsystemState state_;
    FaultSeverity fault_severity_;

    double power_draw_multiplier_;

    std::chrono::steady_clock::time_point transition_start_;

    std::chrono::seconds reboot_duration_;
    std::chrono::seconds patch_duration_;

    std::string active_patch_;
};