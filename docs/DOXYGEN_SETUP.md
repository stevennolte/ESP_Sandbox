# Doxygen Installation and Usage Guide

## Installing Doxygen

### Windows Installation

1. **Download Doxygen**
   - Visit: https://www.doxygen.nl/download.html
   - Download the Windows installer (doxygen-x.x.x-setup.exe)

2. **Install Doxygen**
   - Run the installer as Administrator
   - Follow the installation wizard
   - **Important**: Check "Add to PATH" during installation

3. **Verify Installation**
   ```powershell
   doxygen --version
   ```

### Alternative: Using Chocolatey
```powershell
choco install doxygen.install
```

### Alternative: Using Scoop
```powershell
scoop install doxygen
```

## Generating Documentation

### Method 1: Batch Script (Windows)
```cmd
generate_docs.bat
```

### Method 2: PowerShell Script
```powershell
.\generate_docs.ps1
```

### Method 3: Manual Command
```cmd
doxygen Doxyfile
```

## Documentation Structure

After generation, you'll find:

```
docs/
├── html/
│   ├── index.html          # Main documentation page
│   ├── classes.html        # Class list
│   ├── files.html          # File list
│   ├── functions.html      # Function index
│   └── ...                 # Other generated files
└── mainpage.dox           # Documentation source
```

## Viewing Documentation

1. **Open in Browser**
   ```cmd
   start docs\html\index.html
   ```

2. **PowerShell**
   ```powershell
   Start-Process docs\html\index.html
   ```

3. **Direct Navigation**
   - Navigate to `docs/html/` folder
   - Double-click `index.html`

## Customizing Documentation

### Doxyfile Configuration
Key settings in `Doxyfile`:
- `PROJECT_NAME`: Project title
- `PROJECT_BRIEF`: Short description
- `INPUT`: Source directories
- `OUTPUT_DIRECTORY`: Output location
- `GENERATE_HTML`: Enable HTML output

### Adding Documentation
1. **File-level Documentation**
   ```cpp
   /**
    * @file filename.h
    * @brief Brief description
    * @author Author name
    */
   ```

2. **Class Documentation**
   ```cpp
   /**
    * @class ClassName
    * @brief Class description
    */
   ```

3. **Function Documentation**
   ```cpp
   /**
    * @brief Function description
    * @param param1 Parameter description
    * @return Return value description
    */
   ```

## Troubleshooting

### Common Issues

1. **"doxygen not found"**
   - Ensure Doxygen is installed
   - Check if it's in your PATH
   - Restart command prompt/PowerShell

2. **"Permission denied"**
   - Run as Administrator
   - Check file permissions

3. **Empty documentation**
   - Verify INPUT paths in Doxyfile
   - Check source files have documentation comments

### Getting Help
- Doxygen Manual: https://www.doxygen.nl/manual/
- Configuration Reference: https://www.doxygen.nl/manual/config.html
