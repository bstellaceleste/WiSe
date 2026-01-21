#!/bin/bash
# Integration script for WiSe Hack'25 test binaries
# This script runs the adaptive engine on all provided test binaries

set -e

BINARIES_DIR="tests/WiSeHack_25-binaries"
RESULTS_DIR="test_results"
ENGINE="./adaptive_engine"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "======================================================"
echo "  WiSe Hack'25 - Binary Testing Suite"
echo "======================================================"
echo ""

# Check if engine exists
if [ ! -f "$ENGINE" ]; then
    echo -e "${RED}Error: Engine not found. Run 'make' first.${NC}"
    exit 1
fi

# Check if binaries directory exists
if [ ! -d "$BINARIES_DIR" ]; then
    echo -e "${RED}Error: Binaries directory not found: $BINARIES_DIR${NC}"
    echo ""
    echo "Please create the directory and add the WiSe Hack'25 test binaries:"
    echo "  mkdir -p $BINARIES_DIR"
    echo "  cp /path/to/test_binaries/* $BINARIES_DIR/"
    exit 1
fi

# Create results directory
mkdir -p "$RESULTS_DIR"

# Find all binaries
BINARIES=($(find "$BINARIES_DIR" -type f -executable))

if [ ${#BINARIES[@]} -eq 0 ]; then
    echo -e "${YELLOW}Warning: No executable binaries found in $BINARIES_DIR${NC}"
    echo ""
    echo "Please add the test binaries provided by WiSe Hack'25:"
    echo "  - test1 (known binary)"
    echo "  - test2 (known binary)"
    echo "  - test3 (known binary)"
    echo "  - mystery (unknown binary - testing generalization)"
    exit 1
fi

echo -e "${BLUE}Found ${#BINARIES[@]} test binaries${NC}"
echo ""

# Test counter
TOTAL_TESTS=${#BINARIES[@]}
PASSED=0
FAILED=0

# Run engine on each binary
for binary in "${BINARIES[@]}"; do
    BINARY_NAME=$(basename "$binary")
    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${YELLOW}Testing: $BINARY_NAME${NC}"
    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    OUTPUT_FILE="$RESULTS_DIR/${BINARY_NAME}_results.json"
    LOG_FILE="$RESULTS_DIR/${BINARY_NAME}_output.log"

    # Run the adaptive engine
    if $ENGINE --target "$binary" --output "$OUTPUT_FILE" --verbose > "$LOG_FILE" 2>&1; then
        echo -e "${GREEN}✓ Analysis completed successfully${NC}"
        PASSED=$((PASSED + 1))

        # Extract key metrics
        if [ -f "$OUTPUT_FILE" ]; then
            echo ""
            echo "Key Metrics:"

            # Use Python if available for JSON parsing
            if command -v python3 &> /dev/null; then
                python3 - <<EOF
import json
try:
    with open('$OUTPUT_FILE', 'r') as f:
        data = json.load(f)

    print(f"  Functions analyzed: {data.get('total_functions_analyzed', 'N/A')}")
    print(f"  Total calls:        {data.get('total_calls', 'N/A')}")
    print(f"  Total memory ops:   {data.get('total_memory_ops', 'N/A')}")

    if 'statistics' in data:
        stats = data['statistics']
        print(f"  Call mean:          {stats.get('call_mean', 'N/A'):.2f}")
        print(f"  Call threshold:     {stats.get('call_mean', 0) + 2.0 * stats.get('call_stddev', 0):.2f}")

    # Count hotspots
    hotspots = sum(1 for prof in data.get('function_profiles', []) if prof.get('is_hotspot', False))
    print(f"  Hotspots detected:  {hotspots}")

except Exception as e:
    print(f"  Error parsing JSON: {e}")
EOF
            else
                # Fallback: use grep
                echo "  (Install python3 for detailed metrics)"
                grep -o '"total_functions_analyzed": [0-9]*' "$OUTPUT_FILE" || true
                grep -o '"total_calls": [0-9]*' "$OUTPUT_FILE" || true
            fi

            echo ""
            echo "Full results: $OUTPUT_FILE"
            echo "Detailed log: $LOG_FILE"
        fi

    else
        echo -e "${RED}✗ Analysis failed${NC}"
        FAILED=$((FAILED + 1))
        echo "Error log: $LOG_FILE"
        echo ""
        echo "Last 10 lines of error:"
        tail -10 "$LOG_FILE" 2>/dev/null || echo "No log available"
    fi

    echo ""
done

# Summary
echo -e "${BLUE}======================================================"
echo "  Test Summary"
echo -e "======================================================${NC}"
echo -e "Total binaries tested: $TOTAL_TESTS"
echo -e "${GREEN}Passed: $PASSED${NC}"
echo -e "${RED}Failed: $FAILED${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed successfully!${NC}"
    echo ""
    echo "Next steps:"
    echo "  1. Review results in $RESULTS_DIR/"
    echo "  2. Analyze with: python3 scripts/analyze_results.py $RESULTS_DIR/<file>.json"
    echo "  3. Compare adaptive thresholds across different binaries"
    exit 0
else
    echo -e "${RED}✗ Some tests failed. Review logs in $RESULTS_DIR/${NC}"
    exit 1
fi
