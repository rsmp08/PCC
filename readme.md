# Payload Control Computer (PCC)

Lightweight KSP KRPC payload control simulator with subsystem management, fault handling, and dual UI (console + Windows GUI).

## Quick Start

### Using Build Scripts (Recommended)

**PowerShell (Windows):**
```powershell
.\build.ps1
```

**Bash (Linux/macOS/WSL/Git Bash):**
```bash
bash build.sh
```

### Manual Build Commands

**Console Version:**
```bash
g++ -std=c++17 -O2 -I. main.cpp \
  core/SubsystemManager.cpp \
  libs/FaultManager.cpp libs/EventLog.cpp \
  controllers/CommandInterpreter.cpp controllers/PayloadController.cpp \
  subsystems/ADCSGyros.cpp subsystems/SolarArray.cpp subsystems/StarTracker.cpp \
  subsystems/TelemetryTransceiver.cpp subsystems/ThermalControl.cpp subsystems/PayloadSubsystem.cpp \
  ui/Console.cpp \
  -o payload_control.exe
```

**GUI Version (Windows only, no console):**
```bash
g++ -std=c++17 -O2 -I. \
  core/SubsystemManager.cpp \
  libs/FaultManager.cpp libs/EventLog.cpp \
  controllers/CommandInterpreter.cpp controllers/PayloadController.cpp \
  subsystems/ADCSGyros.cpp subsystems/SolarArray.cpp subsystems/StarTracker.cpp \
  subsystems/TelemetryTransceiver.cpp subsystems/ThermalControl.cpp subsystems/PayloadSubsystem.cpp \
  ui/win_entry.cpp ui/main_gui.cpp ui/PccGui.cpp \
  -lgdi32 -luser32 -mwindows \
  -o payload_control_gui.exe
```

**GUI Version (with console for debugging):**
```bash
g++ -std=c++17 -O2 -I. \
  core/SubsystemManager.cpp \
  libs/FaultManager.cpp libs/EventLog.cpp \
  controllers/CommandInterpreter.cpp controllers/PayloadController.cpp \
  subsystems/ADCSGyros.cpp subsystems/SolarArray.cpp subsystems/StarTracker.cpp \
  subsystems/TelemetryTransceiver.cpp subsystems/ThermalControl.cpp subsystems/PayloadSubsystem.cpp \
  ui/entry_main.cpp ui/main_gui.cpp ui/PccGui.cpp \
  -lgdi32 -luser32 \
  -o payload_control_gui_console.exe
```

## Run

**Console version:**
```powershell
.\payload_control.exe
```

**GUI version:**
```powershell
.\payload_control_gui.exe
```

**GUI with debug console:**
```powershell
.\payload_control_gui_console.exe
```

## Requirements

- **Compiler:** GCC 7+ with C++17 support (MinGW-w64 on Windows)
- **Platform:** Windows (for GUI builds; console is cross-platform)
- **Win32 APIs:** GUI builds require `-lgdi32 -luser32` (Windows only)

## Architecture

### Subsystems
- `subsystems/ADCSGyros.cpp` — Attitude Determination & Control System
- `subsystems/SolarArray.cpp` — Solar panel power generation
- `subsystems/StarTracker.cpp` — Star tracking attitude reference
- `subsystems/TelemetryTransceiver.cpp` — Downlink/uplink comms
- `subsystems/ThermalControl.cpp` — Thermal management
- `subsystems/PayloadSubsystem.cpp` — Primary payload systems

### Core Components
- `core/SubsystemManager.cpp` — Orchestrates all subsystems, state management
- `controllers/PayloadController.cpp` — Simulation loops, command dispatch
- `controllers/CommandInterpreter.cpp` — User command parsing
- `libs/FaultManager.cpp` — Fault injection and recovery
- `libs/EventLog.cpp` — Event/telemetry logging

### User Interfaces
- `ui/Console.cpp` + `main.cpp` — Terminal-based engineering console
- `ui/PccGui.cpp` — Windows native GUI (Win32)
- `ui/win_entry.cpp` — GUI entrypoint (no console window)
- `ui/entry_main.cpp` — GUI entrypoint (with console for debugging)

## Build Targets

| Target | File | Platform | Use Case |
|--------|------|----------|----------|
| Console | `payload_control.exe` | Windows/Linux/macOS | Engineering console, logs, commands |
| GUI (clean) | `payload_control_gui.exe` | Windows only | Clean GUI window |
| GUI (debug) | `payload_control_gui_console.exe` | Windows only | GUI + debug console output |

## Troubleshooting

**GUI shows no motion / MET not advancing:**
- Run the debug version to inspect logs: `payload_control_gui_console.exe`
- Ensure `SubsystemManager::beginDeployment()` is called in `ui/main_gui.cpp`

**Build errors about `WinMain` or GUI initialization:**
- Use `ui/win_entry.cpp` with `-mwindows` for clean GUI
- Use `ui/entry_main.cpp` for debug GUI (keeps console)

**Link errors on non-Windows:**
- GUI builds require `-lgdi32 -luser32` and Windows headers
- Console-only builds are platform-independent

## Build Scripts

- `build.ps1` — PowerShell script; runs all builds with colored output
- `build.sh` — POSIX-compatible shell script; skips GUI on non-Windows

## C++ Standard

Default: C++17 (`-std=c++17`). Can be upgraded to C++20 if your toolchain supports it.
