#!/usr/bin/env bash
# Build script for PCC (Payload Control Computer) - POSIX Shell
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"

# Common source files
COMMON_SOURCES="\
  core/SubsystemManager.cpp \
  libs/FaultManager.cpp \
  libs/EventLog.cpp \
  controllers/CommandInterpreter.cpp \
  controllers/PayloadController.cpp \
  subsystems/ADCSGyros.cpp \
  subsystems/SolarArray.cpp \
  subsystems/StarTracker.cpp \
  subsystems/TelemetryTransceiver.cpp \
  subsystems/ThermalControl.cpp \
  subsystems/PayloadSubsystem.cpp"

COMPILER_FLAGS="-std=c++17 -O2 -I."

# Build console version
echo "Building Console (payload_control)..."
g++ $COMPILER_FLAGS $COMMON_SOURCES ui/Console.cpp main.cpp -o payload_control
echo "✓ payload_control"

# Build GUI version (GUI-only, no console)
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
  echo "Building GUI (no console window)..."
  g++ $COMPILER_FLAGS $COMMON_SOURCES ui/win_entry.cpp ui/main_gui.cpp ui/PccGui.cpp -lgdi32 -luser32 -mwindows -o payload_control_gui.exe || echo "⚠ GUI-only build failed (expected on non-Windows)"
  echo "✓ payload_control_gui.exe"

  echo "Building GUI (console attached for debugging)..."
  g++ $COMPILER_FLAGS $COMMON_SOURCES ui/entry_main.cpp ui/main_gui.cpp ui/PccGui.cpp -lgdi32 -luser32 -o payload_control_gui_console.exe || echo "⚠ GUI console build failed"
  echo "✓ payload_control_gui_console.exe"
else
  echo "⚠ Skipping GUI builds (Windows/MinGW required for Win32 API)"
fi

echo ""
echo "Build complete! Executables:"
echo "  - payload_control (console)"
echo "  - payload_control_gui.exe (GUI only, Windows only)"
echo "  - payload_control_gui_console.exe (GUI with console, Windows only)"
