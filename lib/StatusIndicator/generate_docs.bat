@echo off
REM StatusIndicator Library Documentation Generator (Windows)
REM This script generates Doxygen documentation with diagrams for the StatusIndicator library

echo StatusIndicator Library - Documentation Generator
echo =================================================

REM Check if doxygen is installed
where doxygen >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: Doxygen is not installed!
    echo Please install Doxygen first:
    echo   Download from https://www.doxygen.nl/download.html
    echo   Make sure doxygen.exe is in your PATH
    pause
    exit /b 1
)

REM Check if Graphviz (dot) is installed for diagram generation
where dot >nul 2>nul
if %errorlevel% neq 0 (
    echo Warning: Graphviz ^(dot^) is not installed!
    echo Diagrams will not be generated. To enable diagrams, install Graphviz:
    echo   Download from https://graphviz.org/download/
    echo   Make sure dot.exe is in your PATH
    echo.
    echo Continuing without diagrams...
) else (
    echo ✓ Graphviz detected - diagrams will be generated
)

REM Check if we're in the right directory
if not exist "Doxyfile" (
    echo Error: Doxyfile not found!
    echo Please run this script from the StatusIndicator library directory.
    pause
    exit /b 1
)

REM Clean previous documentation
if exist "docs" (
    echo Cleaning previous documentation...
    rmdir /s /q docs
)

REM Generate documentation
echo Generating documentation...
doxygen Doxyfile

REM Check if generation was successful
if %errorlevel% equ 0 (
    echo Documentation generated successfully!
    echo.
    echo Generated documentation includes:
    echo   ✓ Complete API reference
    echo   ✓ Class hierarchy diagrams
    echo   ✓ Call/caller graphs
    echo   ✓ Include dependency graphs
    echo   ✓ Collaboration diagrams
    echo   ✓ Interactive SVG diagrams
    echo.
    echo Open docs\html\index.html in your web browser to view the documentation.
    
    REM Try to open documentation automatically
    if exist "docs\html\index.html" (
        echo Opening documentation in default browser...
        start docs\html\index.html
    )
) else (
    echo Error: Documentation generation failed!
    pause
    exit /b 1
)

pause
