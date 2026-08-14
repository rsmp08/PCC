#pragma once

#define NOMINMAX
#include <windows.h>

#include "../libs/EventLog.hpp"
#include "../libs/FaultManager.hpp"
#include "../core/SubsystemManager.hpp"

#include <functional>
#include <mutex>
#include <string>

struct PccRuntimeState
{
    mutable std::mutex mutex;

    double met_seconds = 0.0;
    double altitude_km = 400.0;
    double orbital_phase_deg = 0.0;
    double signal_dbm = -120.0;
    double solar_generation_w = 0.0;
    double subsystem_draw_w = 0.0;
    std::string mission_phase = "BOOT";
    bool running = true;
};

class PccGui
{
public:
    PccGui(
        PccRuntimeState &runtime,
        SubsystemManager &subsystem_manager,
        FaultManager &fault_manager,
        EventLog &event_log);

    int run();

private:
    static LRESULT CALLBACK windowProc(
        HWND hwnd,
        UINT message,
        WPARAM wparam,
        LPARAM lparam);

    LRESULT handleMessage(
        UINT message,
        WPARAM wparam,
        LPARAM lparam);

    void paint(HDC dc);
    void paintSidebar(HDC dc, int width, int height);
    void paintOverview(HDC dc, int width, int height);
    void paintSubsystems(HDC dc, int width, int height);
    void paintEvents(HDC dc, int width, int height);
    void paintFaults(HDC dc, int width, int height);

    void handleClick(int x, int y);
    void inject(FaultSeverity severity);
    void rebootSelected();
    void patchSelected();

    std::string selectedSubsystemId() const;
    std::shared_ptr<PayloadSubsystem> selectedSubsystem() const;

    static std::wstring widen(const std::string &text);

    void fill(HDC dc, RECT rect, COLORREF color);
    void text(
        HDC dc,
        const std::string &value,
        int x,
        int y,
        int size,
        COLORREF color,
        bool bold = false);
    void line(
        HDC dc,
        int x1,
        int y1,
        int x2,
        int y2,
        COLORREF color,
        int width = 1);
    void panel(HDC dc, RECT rect);
    void button(HDC dc, RECT rect, const std::string &label, bool active = false);
    void progress(
        HDC dc,
        RECT rect,
        double value,
        COLORREF fillColor);

    PccRuntimeState &runtime_;
    SubsystemManager &subsystem_manager_;
    FaultManager &fault_manager_;
    EventLog &event_log_;

    HWND hwnd_ = nullptr;
    HFONT font_ = nullptr;
    HFONT font_bold_ = nullptr;
    HFONT font_large_ = nullptr;

    int page_ = 0;
    int selected_ = 0;
};
