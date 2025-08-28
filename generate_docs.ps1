# Generate Doxygen Documentation for ESP_Sandbox Project
# PowerShell script to generate HTML documentation from source code

Write-Host ""
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "ESP_Sandbox Documentation Generator" -ForegroundColor Cyan  
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host ""

# Check if Doxygen is available
$doxygenPath = Get-Command doxygen -ErrorAction SilentlyContinue

if (-not $doxygenPath) {
    Write-Host "ERROR: Doxygen not found in PATH!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please install Doxygen from: https://www.doxygen.nl/download.html" -ForegroundColor Yellow
    Write-Host "Make sure to add Doxygen to your system PATH." -ForegroundColor Yellow
    Write-Host ""
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "Doxygen found at: $($doxygenPath.Source)" -ForegroundColor Green
Write-Host "Generating documentation..." -ForegroundColor Yellow
Write-Host ""

# Create docs directory if it doesn't exist
if (-not (Test-Path "docs")) {
    New-Item -ItemType Directory -Path "docs" | Out-Null
    Write-Host "Created docs directory" -ForegroundColor Green
}

# Generate documentation
$process = Start-Process -FilePath "doxygen" -ArgumentList "Doxyfile" -Wait -PassThru -NoNewWindow

if ($process.ExitCode -eq 0) {
    Write-Host ""
    Write-Host "================================================================" -ForegroundColor Green
    Write-Host "Documentation generated successfully!" -ForegroundColor Green
    Write-Host "================================================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Output location: docs\html\index.html" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "You can open the documentation by running:" -ForegroundColor Yellow
    Write-Host "  Start-Process docs\html\index.html" -ForegroundColor White
    Write-Host ""
    
    # Ask if user wants to open documentation
    $openDocs = Read-Host "Do you want to open the documentation now? (y/N)"
    if ($openDocs -eq "y" -or $openDocs -eq "Y") {
        Start-Process "docs\html\index.html"
        Write-Host "Opening documentation in default browser..." -ForegroundColor Green
    }
} else {
    Write-Host ""
    Write-Host "================================================================" -ForegroundColor Red
    Write-Host "ERROR: Documentation generation failed!" -ForegroundColor Red
    Write-Host "================================================================" -ForegroundColor Red
    Write-Host ""
    Write-Host "Exit code: $($process.ExitCode)" -ForegroundColor Red
    Write-Host "Please check the error messages above and fix any issues." -ForegroundColor Yellow
}

Write-Host ""
Read-Host "Press Enter to exit"
