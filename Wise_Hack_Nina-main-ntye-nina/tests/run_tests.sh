#!/bin/bash
# Test script for Adaptive Binary Instrumentation Engine

set -e  # Exit on error

ENGINE="./adaptive_engine"
TEST_DIR="tests/test_binaries"
RESULTS_DIR="test_results"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "  Adaptive Engine - Test Suite"
echo "========================================"
echo ""

# Check if engine exists
if [ ! -f "$ENGINE" ]; then
    echo -e "${RED}✗ Error: Engine not found. Please run 'make' first.${NC}"
    exit 1
fi

# Create results directory
mkdir -p "$RESULTS_DIR"

# Test counter
TESTS_PASSED=0
TESTS_FAILED=0

# Test 1: Basic functionality test (simulation mode)
echo -e "${YELLOW}Test 1: Basic Functionality (Simulation Mode)${NC}"
if $ENGINE --target /bin/ls --output "$RESULTS_DIR/test1_results.json" --verbose > "$RESULTS_DIR/test1_output.txt" 2>&1; then
    echo -e "${GREEN}✓ Test 1 PASSED${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))

    # Check if JSON was created
    if [ -f "$RESULTS_DIR/test1_results.json" ]; then
        echo "  - JSON output created successfully"
    else
        echo -e "${RED}  - Warning: JSON output not found${NC}"
    fi
else
    echo -e "${RED}✗ Test 1 FAILED${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
echo ""

# Test 2: Adaptive threshold calculation
echo -e "${YELLOW}Test 2: Adaptive Threshold Calculation${NC}"
if $ENGINE --target /bin/ls --threshold auto --sensitivity 0.8 --output "$RESULTS_DIR/test2_results.json" > "$RESULTS_DIR/test2_output.txt" 2>&1; then
    echo -e "${GREEN}✓ Test 2 PASSED${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))

    # Verify adaptive behavior
    if grep -q "adaptive" "$RESULTS_DIR/test2_output.txt"; then
        echo "  - Adaptive thresholds detected"
    fi
else
    echo -e "${RED}✗ Test 2 FAILED${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
echo ""

# Test 3: Strategy modes
for strategy in "conservative" "balanced" "aggressive"; do
    echo -e "${YELLOW}Test 3.$strategy: Strategy Mode - $strategy${NC}"
    if $ENGINE --target /bin/ls --strategy "$strategy" --output "$RESULTS_DIR/test3_${strategy}_results.json" > "$RESULTS_DIR/test3_${strategy}_output.txt" 2>&1; then
        echo -e "${GREEN}✓ Test 3.$strategy PASSED${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ Test 3.$strategy FAILED${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
done
echo ""

# Test 4: JSON output validation
echo -e "${YELLOW}Test 4: JSON Output Validation${NC}"
if command -v python3 &> /dev/null; then
    python3 - <<EOF
import json
import sys

try:
    with open('$RESULTS_DIR/test1_results.json', 'r') as f:
        data = json.load(f)

    # Validate required fields
    required_fields = ['engine_version', 'total_functions_analyzed', 'statistics', 'function_profiles']

    for field in required_fields:
        if field not in data:
            print(f"Missing required field: {field}")
            sys.exit(1)

    print("JSON structure valid ✓")
    print(f"Functions analyzed: {data['total_functions_analyzed']}")
    print(f"Total calls: {data['total_calls']}")

    sys.exit(0)
except Exception as e:
    print(f"JSON validation failed: {e}")
    sys.exit(1)
EOF

    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Test 4 PASSED${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ Test 4 FAILED${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
else
    echo -e "${YELLOW}⊘ Test 4 SKIPPED (Python3 not available)${NC}"
fi
echo ""

# Summary
echo "========================================"
echo "  Test Results Summary"
echo "========================================"
echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"
echo "Total Tests:  $((TESTS_PASSED + TESTS_FAILED))"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed! ✓${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed. Check output in $RESULTS_DIR/${NC}"
    exit 1
fi
