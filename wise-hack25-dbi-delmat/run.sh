#!/bin/bash
# run.sh - Script de lancement pour WiSe Hack'25

set -e  # Exit on error

echo "=== WiSe Hack'25 - Adaptive DBI Engine ==="
echo ""

# Check Makefile exists
if [ ! -f "Makefile" ]; then
    echo "ERROR: Makefile not found in current directory"
    exit 1
fi

# Check and build
echo "1. Checking DynamoRIO installation..."
make check-dynamorio

echo ""
echo "2. Building engine and samples..."
make clean
make all
make copy-samples

echo ""
echo "3. List of available binaries:"
echo "   Source location: test_binaries/"
echo "   Copy location:   samples/"
ls -la test_binaries/ 2>/dev/null || echo "test_binaries/ directory not found"
ls -la samples/ 2>/dev/null || echo "samples/ directory not found"

echo ""
echo "4. Running quick tests..."
echo "========================================="
make test

echo ""
echo "5. Configuration options:"
echo "========================================="
echo "To customize engine behavior, use environment variables:"
echo ""
echo "   DBI_VERBOSE=1          # Enable detailed logging"
echo "   DBI_HOTSPOT_MULT=1.5   # Lower hotspot threshold"
echo "   DBI_MIN_CALLS=30       # Faster adaptation"
echo "   DBI_TRACK_MEMORY=0     # Disable memory tracking"
echo ""
echo "Example:"
echo "   DBI_VERBOSE=1 make run-memory"
echo ""
echo "Individual test commands:"
echo "   make run-memory"
echo "   make run-logic"
echo "   make run-balanced"
echo ""
echo "For debugging:"
echo "   make test-verbose"
echo "   make debug  # Build with debug symbols"
echo "========================================="

echo ""
echo "Ready for WiSe Hack'25!"
echo "Use 'make help' for more options"