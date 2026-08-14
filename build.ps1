# Build script for PCC (Payload Control Computer) - PowerShell
$ErrorActionPreference = 'Stop'

Push-Location $PSScriptRoot

try {

    # ============================================================
    # Common source files
    # ============================================================

    $CommonSources = @(
        'core/SubsystemManager.cpp'
        'libs/FaultManager.cpp'
        'libs/EventLog.cpp'
        'controllers/CommandInterpreter.cpp'
        'controllers/PayloadController.cpp'

        'subsystems/ADCSGyros.cpp'
        'subsystems/SolarArray.cpp'
        'subsystems/StarTracker.cpp'
        'subsystems/TelemetryTransceiver.cpp'
        'subsystems/ThermalControl.cpp'
        'subsystems/PayloadSubsystem.cpp'
    )

    $CompilerFlags = @(
        '-std=c++17'
        '-O2'
        '-I.'
    )

    # ============================================================
    # Build console version
    # ============================================================

    Write-Host "Building Console (payload_control.exe)..." -ForegroundColor Cyan

    $ConsoleSources = $CommonSources + @(
        'ui/Console.cpp'
        'main.cpp'
    )

    & g++ @CompilerFlags @ConsoleSources -o payload_control.exe

    if ($LASTEXITCODE -ne 0) {
        throw "Console build failed."
    }

    Write-Host "payload_control.exe built successfully." -ForegroundColor Green

    # ============================================================
    # Build GUI version
    # ============================================================

    Write-Host "Building GUI (no console window)..." -ForegroundColor Cyan

    $GuiSources = $CommonSources + @(
        'ui/win_entry.cpp'
        'ui/main_gui.cpp'
        'ui/PccGui.cpp'
    )

    & g++ @CompilerFlags @GuiSources `
        -lgdi32 `
        -luser32 `
        -mwindows `
        -o payload_control_gui.exe

    if ($LASTEXITCODE -ne 0) {
        throw "GUI build failed."
    }

    Write-Host "payload_control_gui.exe built successfully." -ForegroundColor Green

    # ============================================================
    # Build GUI version with console
    # ============================================================

    Write-Host "Building GUI (console attached for debugging)..." -ForegroundColor Cyan

    $GuiConsoleSources = $CommonSources + @(
        'ui/entry_main.cpp'
        'ui/main_gui.cpp'
        'ui/PccGui.cpp'
    )

    & g++ @CompilerFlags @GuiConsoleSources `
        -lgdi32 `
        -luser32 `
        -o payload_control_gui_console.exe

    if ($LASTEXITCODE -ne 0) {
        throw "GUI console build failed."
    }

    Write-Host "payload_control_gui_console.exe built successfully." -ForegroundColor Green

    # ============================================================
    # Finished
    # ============================================================

    Write-Host ""
    Write-Host "Build complete! Executables:" -ForegroundColor Green
    Write-Host "  - payload_control.exe (console)" -ForegroundColor Yellow
    Write-Host "  - payload_control_gui.exe (GUI only)" -ForegroundColor Yellow
    Write-Host "  - payload_control_gui_console.exe (GUI with console)" -ForegroundColor Yellow

}
finally {
    Pop-Location
}