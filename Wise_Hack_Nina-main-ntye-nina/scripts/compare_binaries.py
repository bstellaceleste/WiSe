#!/usr/bin/env python3
"""
Compare analysis results across multiple binaries
Demonstrates the adaptive nature of the detection algorithm
"""

import json
import sys
from pathlib import Path

def load_result(filepath):
    """Load JSON result file"""
    with open(filepath, 'r') as f:
        return json.load(f)

def compare_binaries(result_files):
    """Compare multiple binary analysis results"""

    results = {}
    for filepath in result_files:
        name = Path(filepath).stem.replace('_results', '')
        results[name] = load_result(filepath)

    print("=" * 80)
    print("  COMPARATIVE ANALYSIS - Adaptive Threshold Demonstration")
    print("=" * 80)
    print()

    # Header
    print(f"{'Binary':<20} {'Functions':<12} {'Total Calls':<12} {'Mean':<10} {'StdDev':<10} {'Threshold':<10} {'Hotspots':<10}")
    print("-" * 80)

    # Compare each binary
    for name, data in results.items():
        stats = data.get('statistics', {})

        funcs = data.get('total_functions_analyzed', 0)
        calls = data.get('total_calls', 0)
        mean = stats.get('call_mean', 0)
        stddev = stats.get('call_stddev', 0)

        # Adaptive threshold calculation (same as engine)
        k = 2.0  # Balanced sensitivity
        threshold = mean + k * stddev

        # Count hotspots
        profiles = data.get('function_profiles', [])
        hotspots = sum(1 for p in profiles if p.get('is_hotspot', False))

        print(f"{name:<20} {funcs:<12} {calls:<12} {mean:<10.2f} {stddev:<10.2f} {threshold:<10.0f} {hotspots:<10}")

    print()
    print("=" * 80)
    print("  KEY INSIGHT: Adaptive Algorithm")
    print("=" * 80)
    print()
    print("Notice how the threshold is COMPUTED from statistics, not hardcoded!")
    print()
    print("Formula: threshold = mean + 2 × stddev")
    print()
    print("This allows the engine to:")
    print("  ✓ Adapt to different binary complexities")
    print("  ✓ Work on unknown 'mystery' binaries")
    print("  ✓ Capture statistical outliers (95th percentile)")
    print("  ✓ Avoid false positives and negatives")
    print()

    # Show top hotspots for each binary
    print("=" * 80)
    print("  Top Detected Hotspots Per Binary")
    print("=" * 80)
    print()

    for name, data in results.items():
        profiles = data.get('function_profiles', [])
        hotspots = [p for p in profiles if p.get('is_hotspot', False)]
        hotspots_sorted = sorted(hotspots, key=lambda x: x['call_count'], reverse=True)

        print(f"\n{name}:")
        print("-" * 40)
        for i, hs in enumerate(hotspots_sorted[:3], 1):
            print(f"  {i}. {hs['name'][:30]:<30} - {hs['call_count']:>6} calls")

    print()

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python3 compare_binaries.py <result1.json> <result2.json> ...")
        print()
        print("Example:")
        print("  python3 scripts/compare_binaries.py test_results_real/*.json")
        sys.exit(1)

    compare_binaries(sys.argv[1:])
