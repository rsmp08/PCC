# Build script for PCC (Payload Control Computer) - PowerShell
$ErrorActionPreference = 'Stop'

Push-Location $PSScriptRoot

# Common source files
$CommonSources = @(
    'core/SubsystemManager.cpp',
    'libs/FaultManager.cpp',
    'libs/EventLog.cpp',
    'controllers/CommandInterpreter.cpp',
    'controllers/PayloadController.cpp',
    'subsystems/ADCSGyros.cpp',
    'subsystems/SolarArray.cpp',
    'subsystems/StarTracker.cpp',
    'subsystems/TelemetryTransceiver.cpp',
    'subsystems/ThermalControl.cpp',
    'subsystems/PayloadSubsystem.cpp'
)

$CompilerFlags = @('-std=c++17', '-O2', '-I.')
$CommonSources_str = ($CommonSources -join ' ')

# Build console version
Write-Host "Building Console (payload_control.exe)..." -ForegroundColor Cyan
$ConsoleSources = $CommonSources + @('ui/Console.cpp', 'main.cpp')
$Console_str = ($ConsoleSources -join ' ')
& g++ $CompilerFlags $Console_str -o payload_control.exe
Write-Host "✓ payload_control.exe" -ForegroundColor Green

# Build GUI version (GUI-only, no console)
Write-Host "Building GUI (no console window)..." -ForegroundColor Cyan
$GuiSources = $CommonSources + @('ui/win_entry.cpp', 'ui/main_gui.cpp', 'ui/PccGui.cpp')
$Gui_str = ($GuiSources -join ' ')
& g++ $CompilerFlags $Gui_str -lgdi32 -luser32 -mwindows -o payload_control_gui.exe
Write-Host "✓ payload_control_gui.exe" -ForegroundColor Green

# Build GUI version (with console attached for debugging)
Write-Host "Building GUI (console attached for debugging)..." -ForegroundColor Cyan
$GuiConsoleSources = $CommonSources + @('ui/entry_main.cpp', 'ui/main_gui.cpp', 'ui/PccGui.cpp')
$GuiConsole_str = ($GuiConsoleSources -join ' ')
& g++ $CompilerFlags $GuiConsole_str -lgdi32 -luser32 -o payload_control_gui_console.exe
Write-Host "✓ payload_control_gui_console.exe" -ForegroundColor Green

Write-Host "`nBuild complete! Executables:" -ForegroundColor Green
Write-Host "  - payload_control.exe (console)" -ForegroundColor Yellow
Write-Host "  - payload_control_gui.exe (GUI only)" -ForegroundColor Yellow
Write-Host "  - payload_control_gui_console.exe (GUI with console)" -ForegroundColor Yellow

Pop-Location
