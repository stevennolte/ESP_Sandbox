@echo off
REM Generate Doxygen Documentation for ESP_Sandbox Project
REM 
REM This script generates HTML documentation from the source code
REM using Doxygen. Make sure Doxygen is installed and in your PATH.

echo.
echo ================================================================
echo ESP_Sandbox Documentation Generator
echo ================================================================
echo.

REM Check if Doxygen is available
where doxygen >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Doxygen not found in PATH!
    echo.
    echo Please install Doxygen from: https://www.doxygen.nl/download.html
    echo Make sure to add Doxygen to your system PATH.
    echo.
    pause
    exit /b 1
)

echo Doxygen found! Generating documentation...
echo.

REM Create docs directory if it doesn't exist
if not exist docs mkdir docs

REM Generate documentation
doxygen Doxyfile

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ================================================================
    echo Documentation generated successfully!
    echo ================================================================
    echo.
    echo Output location: docs\html\index.html
    echo.
    echo You can open the documentation by running:
    echo   start docs\html\index.html
    echo.
    
    REM Ask if user wants to open documentation
    set /p OPEN_DOCS="Do you want to open the documentation now? (y/N): "
    if /i "%OPEN_DOCS%"=="y" (
        start docs\html\index.html
    )
) else (
    echo.
    echo ================================================================
    echo ERROR: Documentation generation failed!
    echo ================================================================
    echo.
    echo Please check the error messages above and fix any issues.
)

echo.
pause
