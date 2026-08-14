# Payload Control Computer (PCC)

Lightweight simulation of a payload control computer with both a terminal engineering console and a native Windows GUI.

This README is updated for the current repository layout where source files are split into `controllers/`, `core/`, `libs/`, and `ui/` under the `PCC/` folder.

## Requirements

- Windows (MinGW-w64 recommended)
- GCC (supports C++17)

Note: the native GUI uses the Win32 APIs (`user32`, `gdi32`).

## Quick Build (PowerShell, run in `PCC`)

Console (terminal) build:
```powershell
g++ -std=c++17 -O2 -I. main.cpp controllers/CommandInterpreter.cpp controllers/PayloadController.cpp core/Subsystem.cpp core/SubsystemManager.cpp libs/FaultManager.cpp libs/EventLog.cpp ui/Console.cpp -o payload_control.exe
```

GUI build (GUI-only, no console window):
```powershell
g++ -std=c++17 -O2 -I. ui/win_entry.cpp ui/main_gui.cpp ui/PccGui.cpp core/Subsystem.cpp core/SubsystemManager.cpp libs/FaultManager.cpp libs/EventLog.cpp -lgdi32 -luser32 -mwindows -o payload_control_gui.exe
```

Console-attached GUI (recommended for debugging):
```powershell
g++ -std=c++17 -O2 -I. ui/entry_main.cpp ui/main_gui.cpp ui/PccGui.cpp core/Subsystem.cpp core/SubsystemManager.cpp libs/FaultManager.cpp libs/EventLog.cpp -lgdi32 -luser32 -o payload_control_gui_console.exe
```

If your toolchain supports C++20 you may replace `-std=c++17` with `-std=c++20`.

## Run

Console:
```powershell
.\payload_control.exe
```

GUI:
```powershell
.\payload_control_gui.exe
```

## Project layout (high level)

- `controllers/` — command interpreter, payload controller (simulation loops)
- `core/` — `Subsystem` and `SubsystemManager` core logic
- `libs/` — `FaultManager`, `EventLog` and other shared libraries
- `ui/` — console and GUI user interfaces
- `main.cpp` — console entrypoint
- `ui/main_gui.cpp` — GUI entrypoint

## Notes

- The GUI build links to `-lgdi32 -luser32` and should be built on Windows.
- The console build is cross-platform friendly but the current UI code uses ANSI/Win32 console handling on Windows.
- If you want me to run either build or package a simple build script, tell me which target to run.

## Troubleshooting

- GUI shows no motion / MET not advancing: run the console-attached GUI to inspect runtime logs:

```powershell
g++ -std=c++17 -O2 -I. ui/entry_main.cpp ui/main_gui.cpp ui/PccGui.cpp core/Subsystem.cpp core/SubsystemManager.cpp libs/FaultManager.cpp libs/EventLog.cpp -lgdi32 -luser32 -o payload_control_gui_console.exe
.\payload_control_gui_console.exe
```

- If the GUI window opens but no subsystems initialize, ensure `subsystem_manager.beginDeployment()` is called (the GUI entrypoint now starts a background sim thread that does this).

- Build/link errors about `WinMain`: use the GUI-only command with `ui/win_entry.cpp` and `-mwindows`, or use the console-attached entry instead.

## Included helper scripts

- `build.ps1` — PowerShell script to build console and GUI targets.
- `build.sh` — POSIX shell script (attempts Windows GUI builds on Windows hosts).

## Next steps (suggested)

- Add VS Code tasks for quick build/run
- Fix remaining compiler warnings
- Add unit tests for core logic
- Add CI build on Windows
