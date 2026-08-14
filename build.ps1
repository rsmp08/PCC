# Build script for PCC (PowerShell)
$ErrorActionPreference = 'Stop'

Push-Location $PSScriptRoot

Write-Host "Building console (payload_control.exe)..."
g++ -std=c++17 -O2 -I. main.cpp controllers/CommandInterpreter.cpp controllers/PayloadController.cpp core/Subsystem.cpp core/SubsystemManager.cpp libs/FaultManager.cpp libs/EventLog.cpp ui/Console.cpp -o payload_control.exe

Write-Host "Building GUI (GUI-only, no console window)..."
g++ -std=c++17 -O2 -I. ui/win_entry.cpp ui/main_gui.cpp ui/PccGui.cpp core/Subsystem.cpp core/SubsystemManager.cpp libs/FaultManager.cpp libs/EventLog.cpp -lgdi32 -luser32 -mwindows -o payload_control_gui.exe

Write-Host "Building GUI (console attached, for debugging)..."
g++ -std=c++17 -O2 -I. ui/entry_main.cpp ui/main_gui.cpp ui/PccGui.cpp core/Subsystem.cpp core/SubsystemManager.cpp libs/FaultManager.cpp libs/EventLog.cpp -lgdi32 -luser32 -o payload_control_gui_console.exe

Pop-Location
