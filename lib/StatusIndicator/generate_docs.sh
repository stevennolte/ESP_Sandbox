#!/bin/bash

# StatusIndicator Library Documentation Generator
# This script generates Doxygen documentation with diagrams for the StatusIndicator library

echo "StatusIndicator Library - Documentation Generator"
echo "================================================="

# Check if doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Error: Doxygen is not installed!"
    echo "Please install Doxygen first:"
    echo "  Ubuntu/Debian: sudo apt-get install doxygen"
    echo "  macOS: brew install doxygen"
    echo "  Windows: Download from https://www.doxygen.nl/download.html"
    exit 1
fi

# Check if Graphviz (dot) is installed for diagram generation
if ! command -v dot &> /dev/null; then
    echo "Warning: Graphviz (dot) is not installed!"
    echo "Diagrams will not be generated. To enable diagrams, install Graphviz:"
    echo "  Ubuntu/Debian: sudo apt-get install graphviz"
    echo "  macOS: brew install graphviz"
    echo "  Windows: Download from https://graphviz.org/download/"
    echo ""
    echo "Continuing without diagrams..."
else
    echo "✓ Graphviz detected - diagrams will be generated"
fi

# Check if we're in the right directory
if [ ! -f "Doxyfile" ]; then
    echo "Error: Doxyfile not found!"
    echo "Please run this script from the StatusIndicator library directory."
    exit 1
fi

# Clean previous documentation
if [ -d "docs" ]; then
    echo "Cleaning previous documentation..."
    rm -rf docs
fi

# Generate documentation
echo "Generating documentation..."
doxygen Doxyfile

# Check if generation was successful
if [ $? -eq 0 ]; then
    echo "Documentation generated successfully!"
    echo ""
    echo "Generated documentation includes:"
    echo "  ✓ Complete API reference"
    echo "  ✓ Class hierarchy diagrams"
    echo "  ✓ Call/caller graphs"
    echo "  ✓ Include dependency graphs"
    echo "  ✓ Collaboration diagrams"
    echo "  ✓ Interactive SVG diagrams"
    echo ""
    echo "Open docs/html/index.html in your web browser to view the documentation."
    
    # Try to open documentation automatically (optional)
    if command -v xdg-open &> /dev/null; then
        echo "Opening documentation in default browser..."
        xdg-open docs/html/index.html &
    elif command -v open &> /dev/null; then
        echo "Opening documentation in default browser..."
        open docs/html/index.html &
    fi
else
    echo "Error: Documentation generation failed!"
    exit 1
fi
