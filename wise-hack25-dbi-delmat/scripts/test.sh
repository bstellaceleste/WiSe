#!/bin/bash
# test.sh - Script de test simplifié

echo "=== WiSe Hack'25 DBI Engine Test ==="
echo ""

# Vérifier DynamoRIO
if [ -z "$DYNAMORIO_HOME" ]; then
    echo "ERROR: Please set DYNAMORIO_HOME"
    echo "Example: export DYNAMORIO_HOME=/home/user/dynamorio/build"
    exit 1
fi

# Compiler tout
echo "1. Building engine and binaries..."
make clean
make all

echo ""
echo "2. List of binaries in test_binaries/:"
ls -la test_binaries/

echo ""
echo "3. Quick test (5 seconds each)..."
echo "----------------------------------------"

echo "Test 1: Memory focus"
DBI_VERBOSE=1 timeout 5 drrun -c libwise_dbi.so -- test_binaries/wise_memory 2>&1 | \
 grep -E "HOT-PATCH|Redirecting|Successfully|ERROR" | head -10 || true

echo ""
echo "Test 2: Logic focus"
DBI_VERBOSE=1 timeout 5 drrun -c libwise_dbi.so -- test_binaries/wise_logic 2>&1 | \
 grep -E "HOT-PATCH|Redirecting|Successfully|ERROR" | head -10 || true

echo ""
echo "Test 3: Balanced"
DBI_VERBOSE=1 timeout 5 drrun -c libwise_dbi.so -- test_binaries/wise_balanced 2>&1 | \
 grep -E "HOT-PATCH|Redirecting|Successfully|ERROR" | head -10 || true

echo ""
echo "4. To run full tests:"
echo "   make run-memory    # Run memory binary"
echo "   make run-logic     # Run logic binary"
echo "   make run-balanced  # Run balanced binary"
echo ""
echo "5. Configuration examples:"
echo "   DBI_VERBOSE=1 DBI_HOTSPOT_MULT=1.5 make run-memory"
echo ""
echo "=== Test completed ==="