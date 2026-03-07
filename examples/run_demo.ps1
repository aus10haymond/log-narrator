# Log Narrator Demo Script
# This script demonstrates the analysis capabilities of Log Narrator

Write-Host "Log Narrator - Demo Script" -ForegroundColor Cyan
Write-Host "============================" -ForegroundColor Cyan
Write-Host ""

# Check if binary exists
$BinaryPath = ".\build\Release\log_narrator.exe"
if (-not (Test-Path $BinaryPath)) {
    Write-Host "Error: Binary not found at $BinaryPath" -ForegroundColor Red
    Write-Host "Please build the project first:" -ForegroundColor Yellow
    Write-Host "  cmake --build build --config Release" -ForegroundColor Yellow
    exit 1
}

# Demo 1: Restart Loop Detection
Write-Host "[Demo 1] Analyzing Restart Loop Scenario..." -ForegroundColor Green
Write-Host "Input: tests\fixtures\logs\scenario_restart_loop.log"
Write-Host ""

& $BinaryPath --verbose --out demo_output\restart_loop tests\fixtures\logs\scenario_restart_loop.log

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "Analysis complete. Output in: demo_output\restart_loop\" -ForegroundColor Green
    Write-Host ""
} else {
    Write-Host ""
    Write-Host "Analysis failed" -ForegroundColor Red
    Write-Host ""
}

# Demo 2: Retry-to-Timeout Detection  
Write-Host "[Demo 2] Analyzing Retry-to-Timeout Scenario..." -ForegroundColor Green
Write-Host "Input: tests\fixtures\logs\scenario_retry_timeout.log"
Write-Host ""

& $BinaryPath --verbose --out demo_output\retry_timeout tests\fixtures\logs\scenario_retry_timeout.log

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "Analysis complete. Output in: demo_output\retry_timeout\" -ForegroundColor Green
    Write-Host ""
} else {
    Write-Host ""
    Write-Host "Analysis failed" -ForegroundColor Red
    Write-Host ""
}

# Demo 3: Error Burst After Config Change
Write-Host "[Demo 3] Analyzing Error Burst After Config Change..." -ForegroundColor Green
Write-Host "Input: tests\fixtures\logs\scenario_config_error_burst.log"
Write-Host ""

& $BinaryPath --verbose --out demo_output\error_burst tests\fixtures\logs\scenario_config_error_burst.log

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "Analysis complete. Output in: demo_output\error_burst\" -ForegroundColor Green
    Write-Host ""
} else {
    Write-Host ""
    Write-Host "Analysis failed" -ForegroundColor Red
    Write-Host ""
}

# Summary
Write-Host ""
Write-Host "================================" -ForegroundColor Cyan
Write-Host "Demo Complete!" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "Output files are in: demo_output\" -ForegroundColor Yellow
Write-Host ""
Write-Host "To view a report, open any of these files:" -ForegroundColor White
Write-Host "  - demo_output\restart_loop\report.md (Markdown)" -ForegroundColor Gray
Write-Host "  - demo_output\restart_loop\report.json (JSON)" -ForegroundColor Gray
Write-Host "  - demo_output\restart_loop\timeline.csv (CSV)" -ForegroundColor Gray
Write-Host ""

Write-Host "Try these commands yourself:" -ForegroundColor White
Write-Host "  .\build\Release\log_narrator.exe --help" -ForegroundColor Cyan
Write-Host "  .\build\Release\log_narrator.exe --verbose yourlogfile.log" -ForegroundColor Cyan
Write-Host "  .\build\Release\log_narrator.exe --format json logs\" -ForegroundColor Cyan
Write-Host ""
