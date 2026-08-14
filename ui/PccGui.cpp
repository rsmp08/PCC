#define NOMINMAX
#include "PccGui.hpp"

#include <windows.h>

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif

#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

namespace
{
    constexpr COLORREF BG = RGB(11, 16, 22);
    constexpr COLORREF PANEL = RGB(18, 25, 33);
    constexpr COLORREF PANEL_2 = RGB(22, 31, 41);
    constexpr COLORREF BORDER = RGB(45, 58, 72);
    constexpr COLORREF TEXT = RGB(224, 232, 240);
    constexpr COLORREF MUTED = RGB(135, 151, 168);
    constexpr COLORREF CYAN = RGB(64, 196, 220);
    constexpr COLORREF GREEN = RGB(71, 205, 128);
    constexpr COLORREF AMBER = RGB(239, 178, 72);
    constexpr COLORREF RED = RGB(235, 82, 82);
    constexpr COLORREF BLUE = RGB(87, 144, 235);

    std::string stateName(SubsystemState state)
    {
        switch (state)
        {
        case SubsystemState::OFFLINE:
            return "OFFLINE";
        case SubsystemState::INITIALIZING:
            return "INITIALIZING";
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

    COLORREF stateColor(SubsystemState state)
    {
        switch (state)
        {
        case SubsystemState::NOMINAL:
            return GREEN;
        case SubsystemState::INITIALIZING:
        case SubsystemState::REBOOTING:
        case SubsystemState::PATCHING:
            return CYAN;
        case SubsystemState::DEGRADED:
            return AMBER;
        case SubsystemState::FAILED:
            return RED;
        case SubsystemState::OFFLINE:
            return MUTED;
        }
        return MUTED;
    }

    double transitionProgress(const PayloadSubsystem &s)
    {
        switch (s.state())
        {
        case SubsystemState::INITIALIZING:
            return s.initializationProgress();
        case SubsystemState::REBOOTING:
            return s.rebootProgress();
        case SubsystemState::PATCHING:
            return s.patchProgress();
        case SubsystemState::NOMINAL:
            return 1.0;
        default:
            return 0.0;
        }
    }

    std::string fmt(double value, int precision = 1)
    {
        std::ostringstream ss;
        ss.setf(std::ios::fixed);
        ss.precision(precision);
        ss << value;
        return ss.str();
    }
}

PccGui::PccGui(
    PccRuntimeState &runtime,
    SubsystemManager &subsystem_manager,
    FaultManager &fault_manager,
    EventLog &event_log)
    : runtime_(runtime),
      subsystem_manager_(subsystem_manager),
      fault_manager_(fault_manager),
      event_log_(event_log)
{
}

std::wstring PccGui::widen(const std::string &value)
{
    if (value.empty())
        return {};

    int count = MultiByteToWideChar(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
        nullptr, 0);

    std::wstring result(count, L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
        result.data(), count);

    return result;
}

void PccGui::fill(HDC dc, RECT rect, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

void PccGui::text(
    HDC dc,
    const std::string &value,
    int x,
    int y,
    int size,
    COLORREF color,
    bool bold)
{
    HFONT old = static_cast<HFONT>(
        SelectObject(dc, bold ? font_bold_ : font_));

    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);

    const auto w = widen(value);
    TextOutW(
        dc, x, y, w.c_str(), static_cast<int>(w.size()));

    SelectObject(dc, old);
}

void PccGui::line(
    HDC dc,
    int x1, int y1, int x2, int y2,
    COLORREF color,
    int width)
{
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HPEN old = static_cast<HPEN>(SelectObject(dc, pen));

    MoveToEx(dc, x1, y1, nullptr);
    LineTo(dc, x2, y2);

    SelectObject(dc, old);
    DeleteObject(pen);
}

void PccGui::panel(HDC dc, RECT rect)
{
    fill(dc, rect, PANEL);

    HPEN pen = CreatePen(PS_SOLID, 1, BORDER);
    HBRUSH oldBrush = static_cast<HBRUSH>(
        SelectObject(dc, GetStockObject(HOLLOW_BRUSH)));
    HPEN oldPen = static_cast<HPEN>(SelectObject(dc, pen));

    Rectangle(
        dc, rect.left, rect.top, rect.right, rect.bottom);

    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
}

void PccGui::button(
    HDC dc,
    RECT rect,
    const std::string &label,
    bool active)
{
    fill(dc, rect, active ? RGB(31, 57, 70) : PANEL_2);

    HPEN pen = CreatePen(
        PS_SOLID, 1, active ? CYAN : BORDER);
    HBRUSH oldBrush = static_cast<HBRUSH>(
        SelectObject(dc, GetStockObject(HOLLOW_BRUSH)));
    HPEN oldPen = static_cast<HPEN>(SelectObject(dc, pen));

    Rectangle(
        dc, rect.left, rect.top, rect.right, rect.bottom);

    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);

    HFONT oldFont = static_cast<HFONT>(
        SelectObject(dc, font_bold_));

    SetTextColor(dc, active ? CYAN : TEXT);
    SetBkMode(dc, TRANSPARENT);

    const auto w = widen(label);
    SIZE sz{};
    GetTextExtentPoint32W(
        dc, w.c_str(), static_cast<int>(w.size()), &sz);

    TextOutW(
        dc,
        rect.left + (rect.right - rect.left - sz.cx) / 2,
        rect.top + (rect.bottom - rect.top - sz.cy) / 2,
        w.c_str(),
        static_cast<int>(w.size()));

    SelectObject(dc, oldFont);
}

void PccGui::progress(
    HDC dc,
    RECT rect,
    double value,
    COLORREF fillColor)
{
    value = std::clamp(value, 0.0, 1.0);
    fill(dc, rect, RGB(32, 42, 53));

    RECT filled = rect;
    filled.right =
        filled.left +
        static_cast<int>((rect.right - rect.left) * value);

    fill(dc, filled, fillColor);
}

int PccGui::run()
{
    WNDCLASSW wc{};
    wc.lpfnWndProc = &PccGui::windowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"PCCMissionControl";
    wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(BG);

    RegisterClassW(&wc);

    hwnd_ = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"Payload Control Computer | Mission Control",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1420, 900,
        nullptr, nullptr,
        wc.hInstance,
        this);

    if (!hwnd_)
        return 1;

    font_ = CreateFontW(
        -16, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");

    font_bold_ = CreateFontW(
        -16, 0, 0, 0, FW_SEMIBOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");

    font_large_ = CreateFontW(
        -28, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");

    SetTimer(hwnd_, 1, 250, nullptr);

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (font_)
        DeleteObject(font_);
    if (font_bold_)
        DeleteObject(font_bold_);
    if (font_large_)
        DeleteObject(font_large_);

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK PccGui::windowProc(
    HWND hwnd,
    UINT message,
    WPARAM wparam,
    LPARAM lparam)
{
    PccGui *self = reinterpret_cast<PccGui *>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE)
    {
        auto *create =
            reinterpret_cast<CREATESTRUCTW *>(lparam);
        self = static_cast<PccGui *>(create->lpCreateParams);

        SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(self));

        self->hwnd_ = hwnd;
    }

    if (self)
        return self->handleMessage(message, wparam, lparam);

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

LRESULT PccGui::handleMessage(
    UINT message,
    WPARAM wparam,
    LPARAM lparam)
{
    switch (message)
    {
    case WM_TIMER:
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_LBUTTONDOWN:
        handleClick(
            GET_X_LPARAM(lparam),
            GET_Y_LPARAM(lparam));
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(hwnd_, &ps);
        paint(dc);
        EndPaint(hwnd_, &ps);
        return 0;
    }

    case WM_DESTROY:
    {
        KillTimer(hwnd_, 1);

        {
            std::lock_guard lock(runtime_.mutex);
            runtime_.running = false;
        }

        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProcW(
        hwnd_, message, wparam, lparam);
}

void PccGui::paint(HDC dc)
{
    RECT client{};
    GetClientRect(hwnd_, &client);

    fill(dc, client, BG);

    const int width = client.right - client.left;
    const int height = client.bottom - client.top;

    paintSidebar(dc, width, height);

    if (page_ == 0)
        paintOverview(dc, width, height);
    else if (page_ == 1)
        paintSubsystems(dc, width, height);
    else if (page_ == 2)
        paintFaults(dc, width, height);
    else
        paintEvents(dc, width, height);
}

void PccGui::paintSidebar(HDC dc, int width, int height)
{
    RECT sidebar{0, 0, 220, height};
    fill(dc, sidebar, RGB(14, 20, 27));

    text(dc, "PAYLOAD", 28, 30, 22, TEXT, true);
    text(dc, "CONTROL COMPUTER", 28, 58, 13, CYAN, true);
    text(dc, "STAGE 2 / FDIR", 28, 82, 12, MUTED);

    line(dc, 24, 108, 196, 108, BORDER);

    const char *pages[] =
        {
            "OVERVIEW",
            "SUBSYSTEMS",
            "FAULT MANAGEMENT",
            "EVENT LOG"};

    for (int i = 0; i < 4; ++i)
    {
        RECT r{
            18,
            128 + i * 54,
            202,
            170 + i * 54};

        if (page_ == i)
            fill(dc, r, RGB(26, 48, 60));

        text(
            dc,
            pages[i],
            34,
            r.top + 15,
            13,
            page_ == i ? CYAN : TEXT,
            page_ == i);
    }

    line(dc, 24, height - 86, 196, height - 86, BORDER);

    text(dc, "SYSTEM", 28, height - 65, 11, MUTED, true);
    text(dc, "● ONLINE", 28, height - 42, 13, GREEN, true);

    RECT exit{28, height - 34, 192, height - 8};
    button(dc, exit, "CLOSE MISSION CONTROL");
}

void PccGui::paintOverview(HDC dc, int width, int height)
{
    const int x = 250;

    PccRuntimeState snapshot;
    {
        std::lock_guard lock(runtime_.mutex);
        snapshot.met_seconds = runtime_.met_seconds;
        snapshot.altitude_km = runtime_.altitude_km;
        snapshot.orbital_phase_deg = runtime_.orbital_phase_deg;
        snapshot.signal_dbm = runtime_.signal_dbm;
        snapshot.solar_generation_w = runtime_.solar_generation_w;
        snapshot.subsystem_draw_w = runtime_.subsystem_draw_w;
        snapshot.mission_phase = runtime_.mission_phase;
    }

    text(dc, "MISSION OVERVIEW", x, 30, 26, TEXT, true);
    text(dc, "LIVE FLIGHT COMPUTER TELEMETRY", x, 65, 12, MUTED);

    const double net =
        snapshot.solar_generation_w -
        snapshot.subsystem_draw_w;

    RECT status{x, 95, width - 30, 160};
    panel(dc, status);

    text(dc, "MISSION PHASE", x + 20, 113, 11, MUTED, true);
    text(dc, snapshot.mission_phase, x + 20, 132, 18, CYAN, true);

    text(dc, "MISSION ELAPSED TIME", x + 410, 113, 11, MUTED, true);
    text(dc, fmt(snapshot.met_seconds, 0) + " s",
         x + 410, 132, 18, TEXT, true);

    text(dc, "SYSTEM", x + 720, 113, 11, MUTED, true);
    text(dc, "NOMINAL", x + 720, 132, 18, GREEN, true);

    RECT orbit{x, 180, x + 500, 470};
    panel(dc, orbit);

    text(dc, "ORBITAL TELEMETRY", x + 20, 200, 14, TEXT, true);

    const int cx = x + 250;
    const int cy = 320;
    const int radius = 110;

    HPEN orbitPen = CreatePen(PS_SOLID, 1, BORDER);
    HPEN oldPen = static_cast<HPEN>(
        SelectObject(dc, orbitPen));
    HBRUSH oldBrush = static_cast<HBRUSH>(
        SelectObject(dc, GetStockObject(HOLLOW_BRUSH)));

    Ellipse(
        dc,
        cx - radius, cy - radius,
        cx + radius, cy + radius);

    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(orbitPen);

    const double a =
        snapshot.orbital_phase_deg * 3.141592653589793 / 180.0;

    const int sx =
        cx + static_cast<int>(std::cos(a) * radius);
    const int sy =
        cy - static_cast<int>(std::sin(a) * radius);

    HBRUSH dot = CreateSolidBrush(CYAN);
    HBRUSH old = static_cast<HBRUSH>(SelectObject(dc, dot));
    Ellipse(dc, sx - 6, sy - 6, sx + 6, sy + 6);
    SelectObject(dc, old);
    DeleteObject(dot);

    text(dc, "ALTITUDE", x + 20, 405, 10, MUTED, true);
    text(dc, fmt(snapshot.altitude_km, 2) + " km",
         x + 20, 423, 17, TEXT, true);

    text(dc, "ORBIT PHASE", x + 190, 405, 10, MUTED, true);
    text(dc, fmt(snapshot.orbital_phase_deg, 1) + " deg",
         x + 190, 423, 17, TEXT, true);

    text(dc, "SIGNAL", x + 350, 405, 10, MUTED, true);
    text(dc, fmt(snapshot.signal_dbm, 1) + " dBm",
         x + 350, 423, 17, TEXT, true);

    RECT power{x + 520, 180, width - 30, 470};
    panel(dc, power);

    text(dc, "POWER SYSTEM", x + 540, 200, 14, TEXT, true);

    text(dc, "SOLAR GENERATION", x + 540, 240, 11, MUTED, true);
    text(dc, fmt(snapshot.solar_generation_w, 1) + " W",
         x + 540, 262, 24, TEXT, true);

    text(dc, "SUBSYSTEM LOAD", x + 540, 315, 11, MUTED, true);
    text(dc, fmt(snapshot.subsystem_draw_w, 1) + " W",
         x + 540, 337, 24, TEXT, true);

    text(dc, "NET BALANCE", x + 540, 390, 11, MUTED, true);
    text(dc, fmt(net, 1) + " W",
         x + 540, 412, 24, net >= 0 ? GREEN : RED, true);

    text(dc, net >= 0 ? "POWER SURPLUS" : "POWER DEFICIT",
         x + 540, 447, 12, net >= 0 ? GREEN : RED, true);

    RECT systems{x, 490, width - 30, height - 34};
    panel(dc, systems);

    text(dc, "SUBSYSTEM HEALTH", x + 20, 510, 14, TEXT, true);

    auto subsystems = subsystem_manager_.all();
    const int rowHeight = 43;

    for (std::size_t i = 0;
         i < subsystems.size() && i < 6;
         ++i)
    {
        auto &s = *subsystems[i];
        const int y = 540 + static_cast<int>(i) * rowHeight;

        text(dc, s.label(), x + 20, y, 12, TEXT, true);
        text(dc, stateName(s.state()),
             x + 220, y, 12, stateColor(s.state()), true);

        RECT bar{x + 330, y + 3, x + 560, y + 16};
        progress(dc, bar, s.health() / 100.0, stateColor(s.state()));

        text(dc, fmt(s.health(), 0) + "%",
             x + 570, y, 12, TEXT);

        text(dc, s.severityString(),
             x + 650, y, 11,
             s.faultSeverity() == FaultSeverity::NONE ? MUTED : AMBER);
    }
}

void PccGui::paintSubsystems(HDC dc, int width, int height)
{
    const int x = 250;

    text(dc, "SUBSYSTEMS", x, 30, 26, TEXT, true);
    text(dc, "HEALTH, TRANSITIONS AND OPERATOR CONTROL",
         x, 65, 12, MUTED);

    RECT table{x, 95, width - 30, height - 145};
    panel(dc, table);

    text(dc, "SUBSYSTEM", x + 20, 115, 11, MUTED, true);
    text(dc, "STATE", x + 250, 115, 11, MUTED, true);
    text(dc, "HEALTH", x + 390, 115, 11, MUTED, true);
    text(dc, "TRANSITION", x + 540, 115, 11, MUTED, true);
    text(dc, "POWER", x + 825, 115, 11, MUTED, true);

    line(dc, x + 18, 140, width - 50, 140, BORDER);

    auto subsystems = subsystem_manager_.all();

    for (std::size_t i = 0; i < subsystems.size(); ++i)
    {
        auto &s = *subsystems[i];
        const int y =
            160 + static_cast<int>(i) * 75;

        if (static_cast<int>(i) == selected_)
        {
            RECT selected{
                x + 10, y - 10, width - 42, y + 55};
            fill(dc, selected, RGB(24, 39, 50));
        }

        text(dc, s.label(), x + 20, y, 13, TEXT, true);
        text(dc, s.identifier(), x + 20, y + 22, 10, MUTED);

        text(dc, stateName(s.state()),
             x + 250, y + 5, 12, stateColor(s.state()), true);

        text(dc, fmt(s.health(), 0) + "%",
             x + 390, y + 5, 13, TEXT, true);

        RECT bar{x + 540, y + 7, x + 780, y + 19};
        progress(dc, bar, transitionProgress(s), stateColor(s.state()));

        text(dc, fmt(transitionProgress(s) * 100.0, 0) + "%",
             x + 790, y + 3, 11, TEXT);

        text(dc, fmt(s.powerDraw(), 1) + " W",
             x + 825, y + 5, 12, TEXT);

        line(dc, x + 20, y + 47, width - 50, y + 47, BORDER);
    }

    RECT action{x, height - 130, width - 30, height - 30};
    panel(dc, action);

    text(dc, "OPERATOR ACTIONS", x + 20, height - 112, 11, MUTED, true);

    const int by = height - 82;

    button(dc, RECT{x + 20, by, x + 150, by + 34},
           "REBOOT", selectedSubsystem() && (selectedSubsystem()->state() == SubsystemState::FAILED || selectedSubsystem()->state() == SubsystemState::DEGRADED));

    button(dc, RECT{x + 165, by, x + 295, by + 34},
           "PATCH", selectedSubsystem() && (selectedSubsystem()->state() == SubsystemState::FAILED || selectedSubsystem()->state() == SubsystemState::DEGRADED));

    button(dc, RECT{x + 325, by, x + 455, by + 34},
           "GLITCH");

    button(dc, RECT{x + 470, by, x + 600, by + 34},
           "DEGRADE");

    button(dc, RECT{x + 615, by, x + 745, by + 34},
           "CRITICAL");

    text(dc,
         selectedSubsystem()
             ? "Selected: " + selectedSubsystem()->label()
             : "No subsystem selected",
         x + 770, by + 9, 12, CYAN, true);
}

void PccGui::paintFaults(HDC dc, int width, int height)
{
    const int x = 250;

    text(dc, "FAULT MANAGEMENT", x, 30, 26, TEXT, true);
    text(dc, "FAULT INJECTION AND RECOVERY MONITOR",
         x, 65, 12, MUTED);

    RECT left{x, 95, x + 540, height - 30};
    RECT right{x + 565, 95, width - 30, height - 30};

    panel(dc, left);
    panel(dc, right);

    text(dc, "ACTIVE FAULTS", x + 20, 115, 14, TEXT, true);

    auto subsystems = subsystem_manager_.all();
    int y = 155;

    for (const auto &s : subsystems)
    {
        if (s->faultSeverity() == FaultSeverity::NONE)
            continue;

        text(dc, s->label(), x + 20, y, 13, TEXT, true);
        text(dc, s->severityString(),
             x + 280, y, 12, AMBER, true);
        text(dc, fmt(s->health(), 0) + "%",
             x + 400, y, 12, TEXT);

        y += 48;
    }

    if (y == 155)
        text(dc, "NO ACTIVE FAULTS",
             x + 20, y, 13, GREEN, true);

    text(dc, "RECOVERY MODEL", x + 20, height - 160,
         11, MUTED, true);
    text(dc, "Transient glitch     85% reboot recovery",
         x + 20, height - 138, 11, TEXT);
    text(dc, "Component degrade    60% reboot recovery",
         x + 20, height - 118, 11, TEXT);
    text(dc, "Critical fault        20% reboot recovery",
         x + 20, height - 98, 11, TEXT);

    text(dc, "FAULT INJECTION", x + 585, 115, 14, TEXT, true);
    text(dc, "Select a subsystem on the SUBSYSTEMS page.",
         x + 585, 145, 12, MUTED);

    text(dc, "Selected subsystem",
         x + 585, 195, 11, MUTED, true);

    text(dc,
         selectedSubsystem()
             ? selectedSubsystem()->label()
             : "NONE",
         x + 585, 220, 20, CYAN, true);

    const int by = 275;

    button(dc, RECT{x + 585, by, x + 760, by + 42},
           "TRANSIENT GLITCH");

    button(dc, RECT{x + 775, by, x + 950, by + 42},
           "DEGRADATION");

    button(dc, RECT{x + 965, by, x + 1140, by + 42},
           "CRITICAL FAULT");

    button(dc, RECT{x + 585, by + 60, x + 760, by + 42 + 60},
           "REBOOT");

    button(dc, RECT{x + 775, by + 60, x + 950, by + 42 + 60},
           "APPLY PATCH");

    text(dc, "The controls intentionally call the same",
         x + 585, 390, 11, MUTED);
    text(dc, "SubsystemManager / FaultManager paths as",
         x + 585, 410, 11, MUTED);
    text(dc, "the engineering command interface.",
         x + 585, 430, 11, MUTED);
}

void PccGui::paintEvents(HDC dc, int width, int height)
{
    const int x = 250;

    text(dc, "EVENT LOG", x, 30, 26, TEXT, true);
    text(dc, "FDIR AND OPERATOR ACTIVITY",
         x, 65, 12, MUTED);

    RECT box{x, 95, width - 30, height - 30};
    panel(dc, box);

    auto events = event_log_.entries();

    int y = 120;

    for (const auto &event : events)
    {
        text(dc, event, x + 20, y, 12, TEXT);
        y += 34;

        if (y > height - 55)
            break;
    }
}

std::string PccGui::selectedSubsystemId() const
{
    auto subsystems = subsystem_manager_.all();

    if (selected_ < 0 ||
        selected_ >= static_cast<int>(subsystems.size()))
        return {};

    return subsystems[selected_]->identifier();
}

std::shared_ptr<PayloadSubsystem>
PccGui::selectedSubsystem() const
{
    const auto id = selectedSubsystemId();
    if (id.empty())
        return nullptr;

    return subsystem_manager_.find(id);
}

void PccGui::inject(FaultSeverity severity)
{
    const auto id = selectedSubsystemId();

    if (id.empty())
    {
        MessageBoxW(
            hwnd_,
            L"Select a subsystem first.",
            L"PCC",
            MB_OK | MB_ICONWARNING);
        return;
    }

    if (severity == FaultSeverity::CRITICAL_FAULT)
    {
        const int answer = MessageBoxW(
            hwnd_,
            L"Inject a CRITICAL FAULT into the selected subsystem?",
            L"Confirm Fault Injection",
            MB_YESNO | MB_ICONWARNING);

        if (answer != IDYES)
            return;
    }

    fault_manager_.injectFault(id, severity);
}

void PccGui::rebootSelected()
{
    const auto id = selectedSubsystemId();
    if (id.empty())
        return;

    std::string result;
    if (subsystem_manager_.rebootSubsystem(id, result))
        event_log_.add("GUI reboot " + id + " -> " + result);
    else
        event_log_.add("GUI reboot FAILED: " + result);
}

void PccGui::patchSelected()
{
    const auto id = selectedSubsystemId();
    if (id.empty())
        return;

    std::string result;
    if (subsystem_manager_.applyPatchFix(
            id, "AUTO_DIAGNOSTIC", result))
        event_log_.add("GUI patch " + id + " -> " + result);
    else
        event_log_.add("GUI patch FAILED: " + result);
}

void PccGui::handleClick(int x, int y)
{
    RECT client{};
    GetClientRect(hwnd_, &client);
    const int height = client.bottom;
    const int width = client.right;

    if (x < 220)
    {
        if (y >= 128 && y < 182)
        {
            page_ = 0;
            InvalidateRect(hwnd_, nullptr, FALSE);
            return;
        }

        if (y >= 182 && y < 236)
        {
            page_ = 1;
            InvalidateRect(hwnd_, nullptr, FALSE);
            return;
        }

        if (y >= 236 && y < 290)
        {
            page_ = 2;
            InvalidateRect(hwnd_, nullptr, FALSE);
            return;
        }

        if (y >= 290 && y < 344)
        {
            page_ = 3;
            InvalidateRect(hwnd_, nullptr, FALSE);
            return;
        }

        if (y > height - 50)
        {
            DestroyWindow(hwnd_);
            return;
        }
    }

    if (page_ == 1)
    {
        auto subsystems = subsystem_manager_.all();

        for (std::size_t i = 0; i < subsystems.size(); ++i)
        {
            const int rowTop =
                150 + static_cast<int>(i) * 75;

            if (y >= rowTop - 10 &&
                y <= rowTop + 55)
            {
                selected_ = static_cast<int>(i);
                InvalidateRect(hwnd_, nullptr, FALSE);
                return;
            }
        }

        const int by = height - 82;

        if (y >= by && y <= by + 34)
        {
            if (x >= 270 && x < 400)
                rebootSelected();
            else if (x >= 415 && x < 545)
                patchSelected();
            else if (x >= 575 && x < 705)
                inject(FaultSeverity::TRANSIENT_GLITCH);
            else if (x >= 720 && x < 850)
                inject(FaultSeverity::COMPONENT_DEGRADATION);
            else if (x >= 865 && x < 995)
                inject(FaultSeverity::CRITICAL_FAULT);

            InvalidateRect(hwnd_, nullptr, FALSE);
        }
    }

    if (page_ == 2)
    {
        const int bx = 815;
        const int by = 275;

        if (y >= by && y < by + 42)
        {
            if (x >= bx && x < bx + 175)
                inject(FaultSeverity::TRANSIENT_GLITCH);
            else if (x >= bx + 190 && x < bx + 365)
                inject(FaultSeverity::COMPONENT_DEGRADATION);
            else if (x >= bx + 380 && x < bx + 555)
                inject(FaultSeverity::CRITICAL_FAULT);
        }
        else if (y >= by + 60 && y < by + 102)
        {
            if (x >= bx && x < bx + 175)
                rebootSelected();
            else if (x >= bx + 190 && x < bx + 365)
                patchSelected();
        }

        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}
